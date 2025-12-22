#ifndef __EXTRACTSCENELUMINANCE_HLSL__
#define __EXTRACTSCENELUMINANCE_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"

Texture2D<HDR_FORMAT> gi_BackBuffer	: register(t0);
RWStructuredBuffer<ShadingConvention::EyeAdaption::HistogramBin> go_Histogram : register(u0);

groupshared uint gLocalBins[gBinCount]

float GetLuminance(in float3 hdr) {
    return dot(hdr, float3(0.2126f, 0.7152f, 0.0722f));
}

float ConvertToLogSpace(in float lum) {
    return log2(max(lum, 1e-5f));;
}

uint ConvertToBinIndex(in float t) {
    return min(uint(t * gBinCount), gBinCount - 1);
}

[numthreads(
    ShadingConvention::EyeAdaption::ThreadGroup::Default::Width,
    ShadingConvention::EyeAdaption::ThreadGroup::Default::Height,
    ShadingConvention::EyeAdaption::ThreadGroup::Default::Depth)]
void CS(in uint2 DTid : SV_DispatchThreadID, in uint2 GTid : SV_GroupThreadID) {
    const uint ThreadIndex = 
        GTid.x + GTid.y * ShadingConvention::EyeAdaption::ThreadGroup::Default::Width;
    if (ThreadIndex < gBinCount) gLocalBins[ThreadIndex] = 0;
    
    GroupMemoryBarrierWithGroupSync();
    
   if (all(DTid < gTexDim)) {
        const float4 Scene = gi_BackBuffer[DTid];
    
        const float Lum = GetLuminance(Scene.rgb);
        const float LogLum = ConvertToLogSpace(Lum);    
        
        const float t = saturate(
            (LogLum - gMinLogLum) / (gMaxLogLum - gMinLogLum));
        const uint BinIndex = ConvertToBinIndex(t);
        
        InterlockedAdd(gLocalBins[BinIndex], 1);    
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    if (ThreadIndex < gBinCount) {
        InterlockedAdd(go_Histogram[ThreadIndex].Count, gLocalBins[ThreadIndex]);
    }
}

#endif // __EXTRACTSCENELUMINANCE_HLSL__