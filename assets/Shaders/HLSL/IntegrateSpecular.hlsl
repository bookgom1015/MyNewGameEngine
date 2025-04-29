#ifndef __INTEGRATESPECULAR_HLSL__
#define __INTEGRATESPECULAR_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef _FIT_TO_SCREEN_COORD
#define _FIT_TO_SCREEN_COORD
#endif

#include "./../../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../../assets/Shaders/HLSL/Samplers.hlsli"
#include "./../../../../assets/Shaders/HLSL/BRDF.hlsli"

ConstantBuffer<ConstantBuffers::PassCB> cbPass : register(b0);

Texture2D<ShadingConvention::ToneMapping::IntermediateMapFormat>    gi_BackBuffer            : register(t0);
Texture2D<ShadingConvention::GBuffer::AlbedoMapFormat>              gi_AlbedoMap             : register(t1);
Texture2D<ShadingConvention::GBuffer::NormalMapFormat>              gi_NormalMap             : register(t2);
Texture2D<ShadingConvention::DepthStencilBuffer::DepthBufferFormat> gi_DepthMap              : register(t3);
Texture2D<ShadingConvention::GBuffer::SpecularMapFormat>            gi_SpecularMap           : register(t4);
Texture2D<ShadingConvention::GBuffer::RoughnessMetalnessMapFormat>  gi_RoughnessMetalnessMap : register(t5);
Texture2D<ShadingConvention::GBuffer::PositionMapFormat>            gi_PositionMap           : register(t6);

Texture2D gi_AOMap : register(t7);
Texture2D gi_ReflectionMap : register(t8);
Texture2D<ShadingConvention::EnvironmentMap::BrdfLutMapFormat>                      gi_BrdfLutMap            : register(t9);
TextureCube<ShadingConvention::EnvironmentMap::PrefilteredEnvironmentCubeMapFormat> gi_PrefilteredEnvCubeMap : register(t10);

struct VertexOut {
    float4 PosH : SV_Position;
    float2 TexC : TexCoord;
};

FitToScreenVertexShader

FitToScreenMeshShader

HDR_FORMAT PS(in VertexOut pin) : SV_Target {
    const float4 PosW = gi_PositionMap.Sample(gsamLinearClamp, pin.TexC);
    const float3 DiffuseRadiance = gi_BackBuffer.Sample(gsamLinearClamp, pin.TexC).rgb;

    if (!ShadingConvention::GBuffer::IsValidPosition(PosW)) return float4(DiffuseRadiance, 1.f);

    const float3 NormalW = normalize(gi_NormalMap.Sample(gsamLinearClamp, pin.TexC).xyz);

    const float4 Albedo = gi_AlbedoMap.Sample(gsamLinearClamp, pin.TexC);
    const float3 ViewW = normalize(cbPass.EyePosW - PosW.xyz);

    
    const float3 Specular = gi_SpecularMap.Sample(gsamLinearClamp, pin.TexC).rgb;
    const float2 RoughnessMetalness = gi_RoughnessMetalnessMap.Sample(gsamLinearClamp, pin.TexC);
    
    const float Roughness = RoughnessMetalness.r;
    const float Metalness = RoughnessMetalness.g;

    const float3 ToLightW = reflect(-ViewW, NormalW);

    const float3 PrefilteredColor = gi_PrefilteredEnvCubeMap.SampleLevel(gsamLinearClamp, ToLightW, Roughness * (float)ShadingConvention::MipmapGenerator::MaxMipLevel).rgb;

    const float NdotV = max(dot(NormalW, ViewW), 0.f);

    //const float4 Reflection = gi_ReflectionMap.Sample(gsamLinearClamp, pin.TexC);
    const float4 Reflection = (float4)0.f;

    const float Shiness = 1.f - Roughness;
    const float3 FresnelR0 = lerp((float3) 0.08f * Specular, Albedo.rgb, Metalness);

    const float3 kS = FresnelSchlickRoughness(saturate(dot(NormalW, ViewW)), FresnelR0, Roughness);
    const float3 kD = 1.f - kS;

    const float2 Brdf = gi_BrdfLutMap.Sample(gsamLinearClamp, float2(NdotV, Roughness));
    const float3 SpecularBias = (kS * Brdf.x + Brdf.y);
    const float Alpha = Reflection.a;

    const float3 SpecularRadiance = (1.f - Alpha) * PrefilteredColor + Alpha * Reflection.rgb;

    //const float AO = gi_AOMap.Sample(gsamLinearClamp, pin.TexC);
    const float AO = 1.f;

    return float4(DiffuseRadiance + AO * SpecularBias * SpecularRadiance, 1.f);
}

#endif // __INTEGRATESPECULAR_HLSL__