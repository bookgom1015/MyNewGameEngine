#ifndef __CIRCLEOFCONFUSION_HLSL__
#define __CIRCLEOFCONFUSION_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"

ConstantBuffer<ConstantBuffers::PassCB> cbPass : register(b0);

DOF_CircleOfConfusion_RootConstants(b1)

Texture2D<ShadingConvention::DepthStencilBuffer::DepthBufferFormat> gi_DepthMap : register(t0);

RWStructuredBuffer<float> gi_FocalDistanceBuffer : register(u0);
RWTexture2D<ShadingConvention::DOF::CircleOfConfusionMapFormat> go_CoC : register(u1);

[numthreads(
    ShadingConvention::DOF::ThreadGroup::Default::Width,
    ShadingConvention::DOF::ThreadGroup::Default::Height,
    ShadingConvention::DOF::ThreadGroup::Default::Depth)]
void CS(in uint2 DTid : SV_DispatchThreadID) {
    const float ZDepth = gi_DepthMap[DTid];
    const float DepthV = ShaderUtil::NdcDepthToViewDepth(ZDepth, cbPass.Proj);
    
    const float FocalDist = gi_FocalDistanceBuffer[0];
    const float Diff = DepthV - FocalDist;
    const float CoC = clamp(Diff / gFocusRange, -1.f, 1.f);
    
    go_CoC[DTid] = CoC;
}

#endif // __CIRCLEOFCONFUSION_HLSL__