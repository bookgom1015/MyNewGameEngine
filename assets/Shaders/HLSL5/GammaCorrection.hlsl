#ifndef __GAMMACORRECTION_HLSL__
#define __GAMMACORRECTION_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef _FIT_TO_SCREEN_COORD
#define _FIT_TO_SCREEN_COORD
#endif

#include "./../../../inc/Render/DX11/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL5/Samplers.hlsli"

Texture2D<float4> gi_BackBuffer : register(t0);

FitToScreenVertexOut

FitToScreenVertexShader

float4 PS(VertexOut pin) : SV_Target {
    float3 color = gi_BackBuffer.SampleLevel(gsamLinearClamp, pin.TexC, 0).rgb;    
    float3 colorCorrected = pow(color, 1.f / 2.2f);
    
    return float4(colorCorrected, 1.f);
}

#endif // __GAMMACORRECTION_HLSL__