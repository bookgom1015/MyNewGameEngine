#ifndef __TEMPORALSMOOTHING_HLSL__
#define __TEMPORALSMOOTHING_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"

EyeAdaption_TemporalSmoothing_RootConstants(b0) 

RWStructuredBuffer<ShadingConvention::EyeAdaption::Result> gi_AvgLogLum : register(u0);
RWStructuredBuffer<float> go_SmoothedLum                                : register(u1);
RWStructuredBuffer<float> gio_PrevLum                                   : register(u2);

[numthreads(1, 1, 1)]
void CS(in uint2 DTid : SV_DispatchThreadID) {
    const float TargetLum = gi_AvgLogLum[0].AvgLogLum;
    const float PrevLum = gio_PrevLum[0];
                                                                                                                                                                                                                                                                                                
    const float Delta = TargetLum - PrevLum;
    const float Glare = saturate((Delta - 1.f) / 3.f);
    
    const float UpSpeed = lerp(gUpSpeed, gGlareUpSpeed, Glare);

    const float k = (TargetLum > PrevLum) ? UpSpeed : gDownSpeed;
    const float Alpha = 1.f - exp(-k * gDeltaTime);

    const float Smoothed = lerp(PrevLum, TargetLum, Alpha);

    go_SmoothedLum[0] = Smoothed;
    gio_PrevLum[0] = Smoothed;
}

#endif // __TEMPORALSMOOTHING_HLSL__