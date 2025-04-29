#ifndef __TONEMAPPING_HLSL__
#define __TONEMAPPING_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef _FIT_TO_SCREEN_COORD
#define _FIT_TO_SCREEN_COORD
#endif

#include "./../../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../../assets/Shaders/HLSL/Samplers.hlsli"

ToneMapping_Default_RootConstants(b0)

Texture2D<ShadingConvention::ToneMapping::IntermediateMapFormat> gi_IntermediateMap : register(t0);

struct VertexOut {
    float4 PosH : SV_Position;
    float2 TexC : TexCoord;
};

FitToScreenVertexShader

FitToScreenMeshShader

float4 PS(in VertexOut pin) : SV_Target {
    const float3 Color = gi_IntermediateMap.SampleLevel(gsamLinearClamp, pin.TexC, 0).rgb;
    const float3 ColorMapped = (float3)1.f - exp(-Color * gExposure);

    return float4(ColorMapped, 1.f);
}

#endif // __TONEMAPPING_HLSL__