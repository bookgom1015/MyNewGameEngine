#ifndef __CONVERTCUBETOEQUIRECTANGULAR_HLSL__
#define __CONVERTCUBETOEQUIRECTANGULAR_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef _FIT_TO_SCREEN_COORD
#define _FIT_TO_SCREEN_COORD
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"

#include "./../../../assets/Shaders/HLSL/EquirectangularConverter.hlsli"

EquirectangularConverter_ConvCubeToEquirect_RootConstants(b0)

TextureCube<float4> gi_CubeMap : register(t0);

struct VertexOut {
    float4 PosH : SV_POSITION;
    float2 TexC : TEXCOORD;
};

VertexOut VS(in uint vid : SV_VertexID) {
    VertexOut vout = (VertexOut)0;

    vout.TexC = gVertices[vid];
    vout.PosH = ShaderUtil::TexCoordToScreen(vout.TexC);

    return vout;
}

float4 PS(in VertexOut pin) : SV_Target {
    const float3 Direction = EquirectangularConverter::SphericalToCartesian(pin.TexC);
    const float3 Color = gi_CubeMap.SampleLevel(gsamLinearClamp, Direction, gMipLevel).rgb;
    
    return float4(Color, 1.f);
}

#endif // __CONVERTCUBETOEQUIRECTANGULAR_HLSL__