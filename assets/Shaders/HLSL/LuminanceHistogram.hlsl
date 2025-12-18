#ifndef __EXTRACTSCENELUMINANCE_HLSL__
#define __EXTRACTSCENELUMINANCE_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

//#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"

Texture2D<HDR_FORMAT> gi_BackBuffer	: register(t0);

float GetLuminance(float3 hdr) {
    return dot(hdr, float3(0.2126f, 0.7152f, 0.0722f));
}

float ConvertToLogSpace(float lum) {
    return log2(max(lum, 1e-5f));;
}

[numthreads(
    ShadingConvention::EyeAdaption::ThreadGroup::Default::Width,
    ShadingConvention::EyeAdaption::ThreadGroup::Default::Height,
    ShadingConvention::EyeAdaption::ThreadGroup::Default::Depth)]
void CS(in uint2 DTid : SV_DispatchThreadID) {
    const float4 Scene = gi_BackBuffer[DTid];
    const float Lum = GetLuminance(Scene.rgb);
    const float LumLog = ConvertToLogSpace(Lum);
    
    
}

#endif // __EXTRACTSCENELUMINANCE_HLSL__