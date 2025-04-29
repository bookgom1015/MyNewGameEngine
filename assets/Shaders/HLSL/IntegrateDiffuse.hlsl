#ifndef __INTEGRATEDIFFUSE_HLSL__
#define __INTEGRATEDIFFUSE_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef _FIT_TO_SCREEN_COORD
#define _FIT_TO_SCREEN_COORD
#endif

#include "./../../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../../assets/Shaders/HLSL/Samplers.hlsli"
#include "./../../../../assets/Shaders/HLSL/LightingUtil.hlsli"

ConstantBuffer<ConstantBuffers::PassCB> cbPass : register(b0);

Texture2D<ShadingConvention::GBuffer::AlbedoMapFormat>              gi_AlbedoMap             : register(t0);
Texture2D<ShadingConvention::GBuffer::NormalMapFormat>              gi_NormalMap             : register(t1);
Texture2D<ShadingConvention::DepthStencilBuffer::DepthBufferFormat> gi_DepthMap              : register(t2);
Texture2D<ShadingConvention::GBuffer::SpecularMapFormat>            gi_SpecularMap           : register(t3);
Texture2D<ShadingConvention::GBuffer::RoughnessMetalnessMapFormat>  gi_RoughnessMetalnessMap : register(t4);
Texture2D<ShadingConvention::GBuffer::PositionMapFormat>            gi_PositionMap           : register(t5);

Texture2D gi_ShadowMap : register(t6);
Texture2D gi_AOMap : register(t7);
TextureCube<ShadingConvention::EnvironmentMap::DiffuseIrradianceCubeMapFormat> gi_DiffuseIrradianceCubeEnv : register(t8);

struct VertexOut {
    float4 PosH : SV_Position;
    float2 TexC : TexCoord;
};

FitToScreenVertexShader

FitToScreenMeshShader

HDR_FORMAT PS(in VertexOut pin) : SV_Target {
    const float4 PosW = gi_PositionMap.Sample(gsamLinearClamp, pin.TexC);    	
    const float4 Albedo = gi_AlbedoMap.Sample(gsamLinearClamp, pin.TexC);
    const float3 Specular = gi_SpecularMap.Sample(gsamLinearClamp, pin.TexC).rgb;
    const float2 RoughnessMetalness = gi_RoughnessMetalnessMap.Sample(gsamLinearClamp, pin.TexC);
    
    const float Roughness = RoughnessMetalness.r;
    const float Metalness = RoughnessMetalness.g;

    const float Shiness = 1 - Roughness;
    const float3 FresnelR0 = lerp(0.08f * Specular, Albedo.rgb, Metalness);

    Material mat = { Albedo, FresnelR0, Shiness, Metalness };

    float shadowFactor[MaxLights];
	//{
	//	[unroll]
    //    for (uint i = 0; i < MaxLights; ++i)
    //        shadowFactor[i] = 1;
    //}
    //
	//{
    //    uint2 size;
    //    gi_Shadow.GetDimensions(size.x, size.y);
    //
    //    const uint2 id = pin.TexC * size - 0.5;
    //    const uint value = gi_Shadow.Load(uint3(id, 0));
    //
	//	[loop]
    //    for (uint i = 0; i < cb_Pass.LightCount; ++i) {
    //        shadowFactor[i] = GetShiftedShadowValue(value, i);
    //    }
    //}

    const float3 NormalW = normalize(gi_NormalMap.Sample(gsamLinearClamp, pin.TexC).xyz);

    const float3 ViewW = normalize(cbPass.EyePosW - PosW.xyz);
    const float3 DiffuseRadiance = max(ComputeBRDF(cbPass.Lights, mat, PosW.xyz, NormalW, ViewW, shadowFactor, cbPass.LightCount), (float3)0.f);

    const float3 kS = FresnelSchlickRoughness(saturate(dot(NormalW, ViewW)), FresnelR0, Roughness);
    float3 kD = 1.f - kS;
    kD *= (1.f - mat.Metalness);

    const float3 DiffuseIrradiance = gi_DiffuseIrradianceCubeEnv.SampleLevel(gsamLinearClamp, NormalW, 0).rgb;
    const float3 Diffuse = DiffuseIrradiance * Albedo.rgb;
    
    //const float AO = gi_AOMap.SampleLevel(gsamLinearClamp, pin.TexC, 0);
    const float AO = 1.f;

    const float3 AmbientLight = (kD * Diffuse) * AO;
    
    return float4((float3) 0, 1.f);
}

#endif // __INTEGRATEDIFFUSE_HLSL__