#ifndef __TONEMAPPING_HLSL__
#define __TONEMAPPING_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef _FIT_TO_SCREEN_COORD
#define _FIT_TO_SCREEN_COORD
#endif

#include "./../../../inc/Render/DX11/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL5/Samplers.hlsli"

Texture2D<float4> gi_IntermediateMap : register(t0);

FitToScreenVertexOut

FitToScreenVertexShader

float3 TonemapACES(in float3 hdr) {
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    return saturate((hdr * (a * hdr + b)) / (hdr * (c * hdr + d) + e));
}

float4 PS(VertexOut pin) : SV_Target {
    float3 hdr = gi_IntermediateMap.SampleLevel(gsamLinearClamp, pin.TexC, 0).rgb;
    float3 color = hdr * 1.4f;
    
    float3 sdr = TonemapACES(color);    
    
    return float4(sdr, 1.f);
}

#endif // __TONEMAPPING_HLSL__