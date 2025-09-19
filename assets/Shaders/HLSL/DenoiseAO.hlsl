#ifndef __DENOISEAO_HLSL__
#define __DENOISEAO_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/ValuePackaging.hlsli"

#define GAUSSIAN_KERNEL_3X3
#include "./../../../assets/Shaders/HLSL/Kernels.hlsli"

ConstantBuffer<ConstantBuffers::AmbientOcclusionCB> cbAO : register(b0);

SSAO_DenoiseAO_RootConstants(b1)

Texture2D<ShadingConvention::DepthStencilBuffer::DepthBufferFormat> gi_DepthMap     : register(t0);
Texture2D<ShadingConvention::GBuffer::NormalMapFormat>              gi_NormalMap    : register(t1);
Texture2D<ShadingConvention::GBuffer::VelocityMapFormat>            gi_VelocityMap  : register(t2);
Texture2D<ShadingConvention::SSAO::AOMapFormat>                     gi_PrevAOMap    : register(t3);

RWTexture2D<ShadingConvention::SSAO::AOMapFormat>                   gio_CurrAOMap   : register(u0);

static const uint NumValuesToLoadPerRowOrColumn = 
    ShadingConvention::SSAO::ThreadGroup::Default::Width + (FilterKernel::Width - 1);

groupshared int gPackedValueDepthCache[NumValuesToLoadPerRowOrColumn][8];
groupshared float gFilteredResultCache[NumValuesToLoadPerRowOrColumn][8];

static const uint2 gGroupDim = uint2(8, 8);

uint2 GetPixelIndex(in uint2 Gid, in uint2 GTid) {
    const uint2 GroupBase = (Gid / gStep) * gGroupDim * gStep + Gid % gStep;
    const uint2 GroupThreadOffset = GTid * gStep;
    const uint2 sDTid = GroupBase + GroupThreadOffset;
    
    return sDTid;
}

void BlurHorizontally(in uint2 Gid, in uint GI) {
    const uint2 GTid4x16_row0 = uint2(GI % 16, GI / 16);
    const int2 GroupKernelBasePixel = GetPixelIndex(Gid, 0) - int(FilterKernel::Radius * gStep);
    const uint NumRowsToLoadPerThread = 4;
    const uint Row_BaseWaveLaneIndex = (WaveGetLaneIndex() / 16) * 16;
    
    [unroll]
    for (int i = 0; i < NumRowsToLoadPerThread; ++i) {
        const uint2 GTid4x16 = GTid4x16_row0 + uint2(0, i * 4);
        if (GTid4x16.y >= NumValuesToLoadPerRowOrColumn) break;
        
        const int2 pixel = GroupKernelBasePixel + GTid4x16 * gStep;
        float value = ShadingConvention::SSAO::InvalidAOValue;
        float depth = 0.f;
        
        if (GTid4x16.x < NumValuesToLoadPerRowOrColumn && ShaderUtil::IsWithinBounds(pixel, gTextureDim)) {
            value = gio_CurrAOMap[pixel];
            
            const uint2 FullResPixel = pixel * 2;
            depth = gi_DepthMap[FullResPixel];
        }
        
        if (ShaderUtil::IsInRange(GTid4x16.x, FilterKernel::Radius, FilterKernel::Radius + gGroupDim.x - 1)) {
            gPackedValueDepthCache[GTid4x16.y][GTid4x16.x - FilterKernel::Radius] = 
                ValuePackaging::Float2ToHalf(float2(value, depth));
        }
            
        float weightedValueSum = 0.f;
        float gaussianWeightedValueSum = 0.f;
        float weightSum = 0.f;
        float gaussianWeightedSum = 0.f;
        
        const uint Row_KernelStartLaneIndex = (Row_BaseWaveLaneIndex + GTid4x16.x) - (GTid4x16.x < gGroupDim.x
                    ? 0 : gGroupDim.x);
            
        const uint kcLaneIndex = Row_KernelStartLaneIndex + FilterKernel::Radius;
        const float kcValue = WaveReadLaneAt(value, kcLaneIndex);
        const float kcDepth = WaveReadLaneAt(depth, kcLaneIndex);
        
        if (GTid4x16.x < gGroupDim.x && 
                kcValue != ShadingConvention::SSAO::InvalidAOValue &&
                kcDepth != ShadingConvention::DepthStencilBuffer::InvalidDepthValue) {
            const float w_h = FilterKernel::Kernel1D[FilterKernel::Radius];
            gaussianWeightedValueSum = w_h * kcValue;
            gaussianWeightedSum = w_h;
            weightedValueSum = gaussianWeightedValueSum;
            weightSum = w_h;
        }
        
        const uint KernelCellIndexOffset = GTid4x16.x < gGroupDim.x ? 0 : (FilterKernel::Radius + 1);
        
        for (uint c = 0; c < FilterKernel::Radius; ++c) {
            const uint kernelCellIndex = KernelCellIndexOffset + c;

            const uint LaneToReadFrom = Row_KernelStartLaneIndex + kernelCellIndex;
            const float cValue = WaveReadLaneAt(value, LaneToReadFrom);
            const float cDepth = WaveReadLaneAt(depth, LaneToReadFrom);
            
            if (cValue != ShadingConvention::SSAO::InvalidAOValue && 
                    kcDepth != ShadingConvention::DepthStencilBuffer::InvalidDepthValue) {
                const float w_h = FilterKernel::Kernel1D[kernelCellIndex];
                
                const float DepthThreshold = 0.05 + gStep * 0.001 * abs(int(FilterKernel::Radius) - c);
                const float w_d = abs(kcDepth - cDepth) <= DepthThreshold * kcDepth;
                const float w = w_h * w_d;

                weightedValueSum += w * cValue;
                weightSum += w;
                gaussianWeightedValueSum += w_h * cValue;
                gaussianWeightedSum += w_h;
            }
        }
        
        const uint LaneToReadFrom = min(WaveGetLaneCount() - 1, Row_BaseWaveLaneIndex + GTid4x16.x + gGroupDim.x);
        weightedValueSum += WaveReadLaneAt(weightedValueSum, LaneToReadFrom);
        weightSum += WaveReadLaneAt(weightSum, LaneToReadFrom);
        gaussianWeightedValueSum += WaveReadLaneAt(gaussianWeightedValueSum, LaneToReadFrom);
        gaussianWeightedSum += WaveReadLaneAt(gaussianWeightedSum, LaneToReadFrom);
        
        if (GTid4x16.x < gGroupDim.x) {
            const float GaussianFilteredValue = gaussianWeightedSum > 1e-6 ? 
                gaussianWeightedValueSum / gaussianWeightedSum : ShadingConvention::SSAO::InvalidAOValue;
            const float FilteredValue = weightSum > 1e-6 ? weightedValueSum / weightSum : GaussianFilteredValue;

            gFilteredResultCache[GTid4x16.y][GTid4x16.x] = FilteredValue;
        }
    }
}

void BlurVertically(in uint2 DTid, in uint2 GTid) {
    const float2 kcValueDepth = ValuePackaging::HalfToFloat2(
        gPackedValueDepthCache[GTid.y + FilterKernel::Radius][GTid.x]);
    const float kcValue = kcValueDepth.x;
    const float kcDepth = kcValueDepth.y;
    
    float filteredValue = kcValue;
    if (kcDepth != ShadingConvention::DepthStencilBuffer::InvalidDepthValue) {
        float weightedValueSum = 0.f;
        float weightSum = 0.f;
        float gaussianWeightedValueSum = 0.f;
        float gaussianWeightSum = 0.f;
        
        [unroll]
        for (uint r = 0; r < FilterKernel::Width; ++r) {
            const uint RowID = GTid.y + r;

            const float2 rUnpackedValueDepth = ValuePackaging::HalfToFloat2(gPackedValueDepthCache[RowID][GTid.x]);
            const float rDepth = rUnpackedValueDepth.y;
            const float rFilteredValue = gFilteredResultCache[RowID][GTid.x];

            if (rDepth != ShadingConvention::DepthStencilBuffer::InvalidDepthValue && 
                    rFilteredValue != ShadingConvention::SSAO::InvalidAOValue) {
                const float w_h = FilterKernel::Kernel1D[r];

                // Simple depth test with tolerance growing as the kernel radius increases.
                // Goal is to prevent values too far apart to blend together, while having 
                // the test being relaxed enough to get a strong blurring result.
                const float DepthThreshold = 0.05 + gStep * 0.001 * abs(int(FilterKernel::Radius) - int(r));
                const float w_d = abs(kcDepth - rDepth) <= DepthThreshold * kcDepth;
                const float w = w_h * w_d;

                weightedValueSum += w * rFilteredValue;
                weightSum += w;
                gaussianWeightedValueSum += w_h * rFilteredValue;
                gaussianWeightSum += w_h;
            }
            
        }
        
        const float GaussianFilteredValue = gaussianWeightSum > 1e-6 ? 
            gaussianWeightedValueSum / gaussianWeightSum : ShadingConvention::SSAO::InvalidAOValue;
        filteredValue = weightSum > 1e-6 ? weightedValueSum / weightSum : GaussianFilteredValue;
        filteredValue = filteredValue != ShadingConvention::SSAO::InvalidAOValue ? 
            lerp(kcValue, filteredValue, 1.f) : filteredValue;
    }
    
    gio_CurrAOMap[DTid] = filteredValue;
}

[numthreads(
    ShadingConvention::SSAO::ThreadGroup::Default::Width,
    ShadingConvention::SSAO::ThreadGroup::Default::Height,
    ShadingConvention::SSAO::ThreadGroup::Default::Depth)]
void CS(in uint2 DTid : SV_DispatchThreadID, in uint2 Gid : SV_GroupID, 
        in uint2 GTid : SV_GroupThreadID, in uint GI : SV_GroupIndex) {
    const uint2 sDTid = GetPixelIndex(Gid, GTid);
    
    BlurHorizontally(Gid, GI);    
    GroupMemoryBarrierWithGroupSync();
    
    BlurVertically(sDTid, GTid);
    GroupMemoryBarrierWithGroupSync();
    
    const uint2 FullResDTid = DTid * 2;
    
    const float2 Velocity = gi_VelocityMap[FullResDTid];
    const int2 DTidOffset = Velocity * FullResDTid;
    
    const int2 PrevFullResDTid = FullResDTid + DTidOffset;
    if (!ShaderUtil::IsWithinBounds(PrevFullResDTid, gTextureDim * 2)) {
        
    }
    
    const float PrevDepth = gi_DepthMap[PrevFullResDTid];
    const float3 PrevNormal = gi_NormalMap[PrevFullResDTid].xyz;
    
    const float CurrValue = gio_CurrAOMap[DTid];
    if (CurrValue == ShadingConvention::SSAO::InvalidAOValue) {
        gio_CurrAOMap[DTid] = ShadingConvention::SSAO::InvalidAOValue;
        return;
    }    
    
    const float PrevValue = gi_PrevAOMap[DTid];
    if (PrevValue == ShadingConvention::SSAO::InvalidAOValue) {
        gio_CurrAOMap[DTid] = CurrValue;
        return;
    }    
    
    const float InterpolatedValue = lerp(PrevValue, CurrValue, 0.08f);
    
    gio_CurrAOMap[DTid] = InterpolatedValue;
}

#endif // __DENOISEAO_HLSL__