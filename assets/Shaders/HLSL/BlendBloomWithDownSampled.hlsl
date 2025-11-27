#ifndef __BLENDBLOOMWITHDOWNSAMPLED_HLSL__
#define __BLENDBLOOMWITHDOWNSAMPLED_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"

Bloom_BlendBloomWithDownSampled_RootConstants(b0)

Texture2D<ShadingConvention::Bloom::HighlightMapFormat> gi_LowerScaleMap     : register(t0);
RWTexture2D<ShadingConvention::Bloom::HighlightMapFormat> gio_HigherScaleMap : register(u0);

[numthreads(
    ShadingConvention::Bloom::ThreadGroup::Default::Width,
    ShadingConvention::Bloom::ThreadGroup::Default::Height,
    ShadingConvention::Bloom::ThreadGroup::Default::Depth)]
void CS(in uint2 DTid : SV_DispatchThreadID) {
    const float2 TexC = ((float2)DTid + 0.5f) * gInvTexDim;

    const float4 LowSample = gi_LowerScaleMap.SampleLevel(gsamLinearClamp, TexC, 0);
    const float4 HighSample = gio_HigherScaleMap[DTid];
    
    gio_HigherScaleMap[DTid] = lerp(HighSample, LowSample, 0.5f);
}

#endif // __BLENDBLOOMWITHDOWNSAMPLED_HLSL__