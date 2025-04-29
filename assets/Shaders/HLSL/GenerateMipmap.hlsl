#ifndef __GENERATEMIPMAP_HLSL__
#define __GENERATEMIPMAP_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef _FIT_TO_SCREEN_COORD
#define _FIT_TO_SCREEN_COORD
#endif

#include "./../../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../../assets/Shaders/HLSL/Samplers.hlsli"

MipmapGenerator_Default_RootConstants(b0)

Texture2D<float4> gi_InputMap : register(t0);

struct VertexOut {
    float4 PosH : SV_Position;
    float2 TexC : TEXCOORD;
};

FitToScreenVertexShader

FitToScreenMeshShader

float4 PS_GenerateMipmap(in VertexOut pin) : SV_Target {
    const float2 TexC0 = pin.TexC;
    const float2 TexC1 = pin.TexC + float2(gInvTexSize.x, 0.f);
    const float2 TexC2 = pin.TexC + float2(0.f, gInvTexSize.y);
    const float2 TexC3 = pin.TexC + float2(gInvTexSize.x, gInvTexSize.y);

    const float4 Color0 = gi_InputMap.SampleLevel(gsamLinearClamp, TexC0, 0);
    const float4 Color1 = gi_InputMap.SampleLevel(gsamLinearClamp, TexC1, 0);
    const float4 Color2 = gi_InputMap.SampleLevel(gsamLinearClamp, TexC2, 0);
    const float4 Color3 = gi_InputMap.SampleLevel(gsamLinearClamp, TexC3, 0);

    const float4 finalColor = (Color0 + Color1 + Color2 + Color3) * 0.25f;
    
    return finalColor;
}

float4 PS_CopyMap(in VertexOut pin) : SV_Target {
    const float4 Color = gi_InputMap.SampleLevel(gsamLinearClamp, pin.TexC, 0);
    
    return Color;
}

#endif // __GENERATEMIPMAP_HLSL__