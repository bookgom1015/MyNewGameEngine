// [ Descriptions ]
//  Stage 1 of Temporal-Supersampling.
//  Samples temporal cache via vectors/reserve reprojection.
//  If no valid values have been reterived from the cache, the tspp is set to 0.

#ifndef __TEMPORALSUPERSAMPLINGREVERSEREPROJECT_HLSL__
#define __TEMPORALSUPERSAMPLINGREVERSEREPROJECT_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"
#include "./../../../assets/Shaders/HLSL/SVGF.hlsli"
#include "./../../../assets/Shaders/HLSL/ValuePackaging.hlsli"
#include "./../../../assets/Shaders/HLSL/CrossBilateralWeights.hlsli"

ConstantBuffer<ConstantBuffers::SVGF::CrossBilateralFilterCB> cbReproject : register(b0);

SVGF_TemporalSupersamplingReverseReproject_RootConstants(b1)

Texture2D<ShadingConvention::GBuffer::NormalDepthMapFormat>         gi_CurrentFrameNormalDepth : register(t0);
Texture2D<ShadingConvention::GBuffer::NormalDepthMapFormat>         gi_ReprojectedNormalDepth  : register(t1);
Texture2D<ShadingConvention::GBuffer::VelocityMapFormat>            gi_Velocity                : register(t2);
Texture2D<ShadingConvention::SVGF::DepthPartialDerivativeMapFormat> gi_DepthPartialDerivative  : register(t3);
Texture2D<ShadingConvention::GBuffer::NormalDepthMapFormat>         gi_CachedNormalDepth       : register(t4);
Texture2D<ShadingConvention::SVGF::ValueMapFormat_Color>            gi_CachedValue             : register(t5);
Texture2D<ShadingConvention::SVGF::ValueSquaredMeanMapFormat_Color> gi_CachedValueSquaredMean  : register(t6);
Texture2D<ShadingConvention::SVGF::TSPPMapFormat>                   gi_CachedTspp              : register(t7);
Texture2D<ShadingConvention::SVGF::RayHitDistanceFormat>            gi_CachedRayHitDistance    : register(t8);

RWTexture2D<ShadingConvention::SVGF::TSPPMapFormat>                          go_CachedTspp              : register(u0);
RWTexture2D<ShadingConvention::SVGF::ValueMapFormat_Color>                   go_CachedValue             : register(u1);
RWTexture2D<ShadingConvention::SVGF::ValueSquaredMeanMapFormat_Color>        go_CachedSquaredMean       : register(u2);
RWTexture2D<ShadingConvention::SVGF::TSPPSquaredMeanRayHitDistanceMapFormat> go_ReprojectedCachedValues : register(u3);

#include "./../../../assets/Shaders/HLSL/TemporalSupersamplingReverseReproject.hlsli"

[numthreads(
    ShadingConvention::SVGF::ThreadGroup::Default::Width,
    ShadingConvention::SVGF::ThreadGroup::Default::Height,
    ShadingConvention::SVGF::ThreadGroup::Default::Depth)]
void CS(in uint2 DTid : SV_DispatchThreadID) {
    if (!SVGF::IsWithinBounds(DTid, gTexDim)) return;
    
    const uint ReprojNormalDepth = gi_ReprojectedNormalDepth[DTid];
    
    const float2 TexC = (DTid + 0.5) * gInvTexDim;
    const float2 Velocity = gi_Velocity.SampleLevel(gsamLinearClamp, TexC, 0);
    
    if (ShadingConvention::GBuffer::IsValidNormalDepth(ReprojNormalDepth) || Velocity.x > 100.f) {
        go_CachedTspp[DTid] = 0;
        return;
    }
    
    float3 reprojNormal;
    float reprojDepth;
    ValuePackaging::DecodeNormalDepth(ReprojNormalDepth, reprojNormal, reprojDepth);

    const float2 CacheTexC = TexC - Velocity;
	
	// Find the nearest integer index samller than the texture position.
	// The floor() ensures the that value sign is taken into consideration.
    const int2 TopLeftCacheIndex = floor(CacheTexC * gTexDim - 0.5);
    const float2 AdjustedCacheTex = (TopLeftCacheIndex + 0.5) * gInvTexDim;

    const float2 CachePixelOffset = CacheTexC * gTexDim - 0.5 - TopLeftCacheIndex;

    const int2 SrcIndexOffsets[4] = { { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 } };

    const int2 CacheIndices[4] = {
        TopLeftCacheIndex + SrcIndexOffsets[0],
		TopLeftCacheIndex + SrcIndexOffsets[1],
		TopLeftCacheIndex + SrcIndexOffsets[2],
		TopLeftCacheIndex + SrcIndexOffsets[3]
    };

    float3 cacheNormals[4];
    float4 cacheDepths = 0;
	{
        const uint4 Packed = gi_CachedNormalDepth.GatherRed(gsamPointClamp, AdjustedCacheTex).wzxy;
        
		[unroll]
        for (int i = 0; i < 4; ++i) 
            ValuePackaging::DecodeNormalDepth(Packed[i], cacheNormals[i], cacheDepths[i]);
    }

    const float2 Ddxy = gi_DepthPartialDerivative.SampleLevel(gsamPointClamp, TexC, 0);

    float4 weights = TemporalSupersamplingReverseReproject::BilateralResampleWeights(reprojDepth, reprojNormal, cacheDepths, cacheNormals, CachePixelOffset, DTid, CacheIndices, Ddxy);

	// Invalidate weights for invalid values in the cache.
    const float4 vCacheValues = gi_CachedValue.GatherRed(gsamPointClamp, AdjustedCacheTex).wzxy;
    weights = select(vCacheValues != ShadingConvention::SVGF::InvalidContrastValue, weights, 0);
    
    const float WeightSum = dot(1, weights);

    float cachedValue = ShadingConvention::SVGF::InvalidContrastValue;
    float cachedValueSquaredMean = 0;
    float cachedRayHitDist = 0;

    uint tspp;
    if (WeightSum > 0.001) {
        uint4 vCachedTspp = gi_CachedTspp.GatherRed(gsamPointClamp, AdjustedCacheTex).wzxy;
		// Enforce tspp of at least 1 for reprojection for valid values.
		// This is because the denoiser will fill in invalid values with filtered 
		// ones if it can. But it doesn't increase tspp.
        vCachedTspp = max(1, vCachedTspp);

        const float4 nWeights = weights / WeightSum; // Normalize the weights.

		// Scale the tspp by the total weight. This is to keep the tspp low for 
		// total contributions that have very low reprojection weight. While its preferred to get 
		// a weighted value even for reprojections that have low weights but still
		// satisfy consistency tests, the tspp needs to be kept small so that the Target calculated values
		// are quickly filled in over a few frames. Otherwise, bad estimates from reprojections,
		// such as on disocclussions of surfaces on rotation, are kept around long enough to create 
		// visible streaks that fade away very slow.
		// Example: rotating camera around dragon's nose up close. 
        const float TsppScale = 1;

        const float CachedTspp = TsppScale * dot(nWeights, vCachedTspp);
        tspp = round(CachedTspp);

        if (tspp > 0) {
            cachedValue = dot(nWeights, vCacheValues);

            const float4 vCachedValueSquaredMean = gi_CachedValueSquaredMean.GatherRed(gsamPointClamp, AdjustedCacheTex).wzxy;
            cachedValueSquaredMean = dot(nWeights, vCachedValueSquaredMean);

            const float4 vCachedRayHitDist = gi_CachedRayHitDistance.GatherRed(gsamPointClamp, AdjustedCacheTex).wzxy;
            cachedRayHitDist = dot(nWeights, vCachedRayHitDist);
        }
    }
    else {
		// No valid values can be retrieved from the cache.
		// TODO: try a greater cache footprint to find useful samples,
		//   For example a 3x3 pixel cache footprint or use lower mip cache input.
        tspp = 0;
    }

    go_CachedTspp[DTid] = tspp;
    go_CachedValue[DTid] = cachedValue;
    go_CachedSquaredMean[DTid] = cachedValueSquaredMean;
    go_ReprojectedCachedValues[DTid] = uint4(tspp, f32tof16(float3(cachedRayHitDist, 0, 0)));
}

#endif // __TEMPORALSUPERSAMPLINGREVERSEREPROJECT_HLSL__