#ifndef __COMPUTEBRDF_HLSL__
#define __COMPUTEBRDF_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef _FIT_TO_SCREEN_COORD
#define _FIT_TO_SCREEN_COORD
#endif

#include "./../../../inc/Render/DX11/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL5/Samplers.hlsli"
#include "./../../../assets/shaders/HLSL5/BRDF.hlsli"
#include "./../../../assets/Shaders/HLSL5/Shadow.hlsli"

PassCB_register(b0);
LightCB_register(b1);

Texture2D<float4> gi_AlbedoMap             : register(t0);
Texture2D<float4> gi_NormalMap             : register(t1);
Texture2D<float4> gi_PositionMap           : register(t2);
Texture2D<float2> gi_RoughnessMetalnessMap : register(t3);
Texture2D<uint>   gi_ShadowMap             : register(t4);

FitToScreenVertexOut

FitToScreenVertexShader

void CalcShadowFactors(out float shadowFactor, in float2 texc) {    
    uint2 size;
    gi_ShadowMap.GetDimensions(size.x, size.y);
    
    uint2 index = min((uint2)(texc * size + 0.5f), size - 1);                                                                                                                                                                                                                                                                                            
    uint value = gi_ShadowMap.Load(uint3(index, 0));
    
	shadowFactor = Shadow::GetShiftedShadowValue(value, _Light.Index);
}

void CalcShadowFactorsPCF5x5(out float shadowFactor, in float2 texc) {    
    uint2 size;
    gi_ShadowMap.GetDimensions(size.x, size.y);    

    float sum = 0.f;

    float2 pixel = texc * size;
    int2 basePixel = (int2)floor(pixel + 0.5f);

    int radius = 2;
    int diameter = (2 * radius) + 1;
    float invCount = 1.0f / (diameter * diameter);

    [loop]
    for (int dy = -radius; dy <= radius; ++dy) {
        [loop]
        for (int dx = -radius; dx <= radius; ++dx) {
            int2 tap = basePixel + int2(dx, dy);
            tap = clamp(tap, int2(0, 0), int2((int)size.x - 1, (int)size.y - 1));

            const uint Value = gi_ShadowMap.Load(uint3((uint2)tap, 0));

            sum += Shadow::GetShiftedShadowValue(Value, _Light.Index);
        }
    }

    shadowFactor = sum * invCount;
}

float4 PS(VertexOut pin) : SV_Target {
    float4 albedo = gi_AlbedoMap.Sample(gsamLinearClamp, pin.TexC);
    float3 normalW = normalize(gi_NormalMap.Sample(gsamLinearClamp, pin.TexC).xyz);
    float3 posW = gi_PositionMap.Sample(gsamLinearClamp, pin.TexC).xyz;
    float2 RM = gi_RoughnessMetalnessMap.Sample(gsamLinearClamp, pin.TexC);
    
    float shadowFactor = 1.f;
    CalcShadowFactorsPCF5x5(shadowFactor, pin.TexC);
    
    Material mat = { albedo, (float3)0.28f, RM.x, RM.y };
    
    float3 viewW = normalize(EyePosW - posW);
    float3 radiance = ComputeBRDF(_Light, mat, posW, normalW, viewW, shadowFactor);
    
    float3 ambient = (AmbientLight.rgb * AmbientLight.a) * albedo;
    
    return float4(radiance + ambient, 1.f);
}

#endif // __COMPUTEBRDF_HLSL__