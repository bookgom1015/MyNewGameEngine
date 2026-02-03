#ifndef __COMPUTEBRDF_HLSL__
#define __COMPUTEBRDF_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef _FIT_TO_SCREEN_COORD
#define _FIT_TO_SCREEN_COORD
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"
#include "./../../../assets/Shaders/HLSL/LightingUtil.hlsli"
#include "./../../../assets/Shaders/HLSL/Shadow.hlsli"

ConstantBuffer<ConstantBuffers::PassCB> cbPass : register(b0);
ConstantBuffer<ConstantBuffers::LightCB> cbLight : register(b1);

BRDF_ComputeBRDF_RootConstants(b2)

Texture2D<ShadingConvention::GBuffer::AlbedoMapFormat>              gi_AlbedoMap             : register(t0);
Texture2D<ShadingConvention::GBuffer::NormalMapFormat>              gi_NormalMap             : register(t1);
Texture2D<ShadingConvention::DepthStencilBuffer::DepthBufferFormat> gi_DepthMap              : register(t2);
Texture2D<ShadingConvention::GBuffer::SpecularMapFormat>            gi_SpecularMap           : register(t3);
Texture2D<ShadingConvention::GBuffer::RoughnessMetalnessMapFormat>  gi_RoughnessMetalnessMap : register(t4);
Texture2D<ShadingConvention::GBuffer::PositionMapFormat>            gi_PositionMap           : register(t5);
Texture2D<ShadingConvention::Shadow::ShadowMapFormat>               gi_ShadowMap             : register(t6);

FitToScreenVertexOut

FitToScreenVertexShader

FitToScreenMeshShader

HDR_FORMAT PS(in VertexOut pin) : SV_Target {
    const float4 Albedo = gi_AlbedoMap.Sample(gsamLinearClamp, pin.TexC);
    if (Albedo.a < 1e-6f) return (float4)0;
                                                                                                                                                                                                                                                                            
    const float4 PosW = gi_PositionMap.Sample(gsamLinearClamp, pin.TexC);    	
    const float3 Specular = gi_SpecularMap.Sample(gsamLinearClamp, pin.TexC).rgb;
    const float2 RoughnessMetalness = gi_RoughnessMetalnessMap.Sample(gsamLinearClamp, pin.TexC);
    
    const float Roughness = RoughnessMetalness.r;
    const float Metalness = RoughnessMetalness.g;

    const float Shiness = 1 - Roughness;
    const float3 FresnelR0 = lerp(0.08f * Specular, Albedo.rgb, Metalness);

    Material mat = { Albedo, FresnelR0, Shiness, Metalness };

    float shadowFactors[MaxLights];
    [unroll]
    for (uint i = 0; i < MaxLights; ++i) {
        shadowFactors[i] = 1;
    }
    if (gShadowEnabled) {    
        Shadow::CalcShadowFactorsPCF5x5(
            gi_ShadowMap, gTexDim, pin.TexC, cbLight.LightCount, shadowFactors);
    }

    const float3 NormalW = normalize(gi_NormalMap.Sample(gsamLinearClamp, pin.TexC).xyz);

    const float3 ViewW = normalize(cbPass.EyePosW - PosW.xyz);
    const float3 Radiance = ComputeBRDF(cbLight.Lights, mat, PosW.xyz, NormalW, ViewW, shadowFactors, cbLight.LightCount);
        
    return float4(Radiance, 1.f);
}

#endif // __COMPUTEBRDF_HLSL__