#ifndef __COMPUTEBRDF_HLSL__
#define __COMPUTEBRDF_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef HLSL_VERSION_UNDER_6
#define HLSL_VERSION_UNDER_6
#endif

#ifndef _FIT_TO_SCREEN_COORD
#define _FIT_TO_SCREEN_COORD
#endif

#ifndef COOK_TORRANCE
#define COOK_TORRANCE
#endif

#include "./../../../inc/Render/DX11/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"
#include "./../../../assets/Shaders/HLSL/LightingUtil.hlsli"
#include "./../../../assets/Shaders/HLSL/Shadow.hlsli"

PassCB_register(b0);
LightCB_register(b1);

Texture2D<float4> gi_AlbedoMap             : register(t0);
Texture2D<float4> gi_NormalMap             : register(t1);
Texture2D<float4> gi_PositionMap           : register(t2);
Texture2D<float2> gi_RoughnessMetalnessMap : register(t3);
Texture2D<uint>   gi_ShadowMap             : register(t4);

TextureCube<float4> gi_DiffuseIrradianceCubeMap      : register(t5);
TextureCube<float4> gi_PrefilteredEnvironmentCubeMap : register(t6);
Texture2D<float4>   gi_BrdfLutMap                    : register(t7);

FitToScreenVertexOut

FitToScreenVertexShader

float4 PS(VertexOut pin) : SV_Target {
    float4 albedo = gi_AlbedoMap.Sample(gsamLinearClamp, pin.TexC);  
    if (albedo.a == 0.f) return (float4)0;
    
    float3 normalW = normalize(gi_NormalMap.Sample(gsamLinearClamp, pin.TexC).xyz);
    float3 posW = gi_PositionMap.Sample(gsamLinearClamp, pin.TexC).xyz;
    float2 RM = gi_RoughnessMetalnessMap.Sample(gsamLinearClamp, pin.TexC);
    
    float roughness = RM.r;
    float metalness = RM.g;
    
    float shadowFactors[MaxLights];
    [unroll]
    for (uint i = 0; i < MaxLights; ++i) {
        shadowFactors[i] = 1;
    }
    
    {
        uint2 size;
        gi_ShadowMap.GetDimensions(size.x, size.y);
    
        Shadow::CalcShadowFactorsPCF5x5(
            gi_ShadowMap, size, pin.TexC, LightCount, shadowFactors);
    }
    
    const float3 specular = (float3)0.5f;
    const float shiness = 1.f - roughness;
    const float3 fresnelR0 = lerp(0.08f * specular, albedo.rgb, metalness);
    
    Material mat = { albedo, fresnelR0, shiness, metalness };
        
    float3 viewW = normalize(EyePosW - posW);
    float3 radiance = ComputeBRDF(Lights, mat, posW, normalW, viewW, shadowFactors, LightCount);
    
    float3 kS = FresnelSchlickRoughness(saturate(dot(normalW, viewW)), fresnelR0, roughness);
    float3 kD = 1.f - kS;
    kD *= (1.f - metalness);
    
    float3 diffuseIrrad =  gi_DiffuseIrradianceCubeMap.Sample(gsamLinearClamp, normalW).rgb;
    
    float NdotV = saturate(dot(normalW, viewW));
    
    const float2 brdf = gi_BrdfLutMap.Sample(gsamLinearClamp, float2(NdotV, roughness));
    const float3 specBias = (kS * brdf.x + brdf.y);
    
    float3 toLightW = reflect(-viewW, normalW);
    
    float3 specIrrad = gi_PrefilteredEnvironmentCubeMap.SampleLevel(
        gsamLinearClamp, toLightW, roughness * 4.f).rgb;
    
    float3 diffuseAmbient = kD * albedo.rgb * diffuseIrrad;
    float3 specAmbient = specBias * specIrrad;
    float3 ambient = diffuseAmbient + specAmbient;
    
    return float4(radiance + ambient, 1.f);
}

#endif // __COMPUTEBRDF_HLSL__