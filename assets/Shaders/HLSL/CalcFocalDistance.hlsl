#ifndef __CALCFOCALDISTANCE_HLSL__
#define __CALCFOCALDISTANCE_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"

ConstantBuffer<ConstantBuffers::PassCB> cbPass : register(b0);

Texture2D<ShadingConvention::GBuffer::PositionMapFormat> gi_PositionMap : register(t0);

RWStructuredBuffer<float> gio_FocalDistanceBuffer : register(u0);

groupshared float FocalDistanceSet[8 * 8];

[numthreads(
    ShadingConvention::DOF::ThreadGroup::Default::Width,
    ShadingConvention::DOF::ThreadGroup::Default::Height,
    ShadingConvention::DOF::ThreadGroup::Default::Depth)]
void CS(in uint2 DTid : SV_DispatchThreadID) {
    uint2 size;
    gi_PositionMap.GetDimensions(size.x, size.y);
    
    const uint2 HalfSize = size * 0.5f;
    const uint2 Index = HalfSize + (DTid - 4);
    
    const float4 PosW = gi_PositionMap[Index];
    const float4 PosV = mul(PosW, cbPass.View);
    
    FocalDistanceSet[DTid.x + DTid.y * 8] = PosV.z;
    
    GroupMemoryBarrierWithGroupSync();

    if (all(DTid == 0)) {
        const float PrevFocalDist = gio_FocalDistanceBuffer[0];
        
        float sum = 0.f;
        for (uint i = 0; i < 64; ++i) sum += FocalDistanceSet[i];            
        const float Result = sum / 64.f;
        
        gio_FocalDistanceBuffer[0] = lerp(PrevFocalDist, Result, 0.02f);
    }
}

#endif // __CALCFOCALDISTANCE_HLSL__