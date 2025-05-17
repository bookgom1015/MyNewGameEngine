#ifndef __DRAWSHADOW_HLSL__
#define __DRAWSHADOW_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../../assets/Shaders/HLSL/Samplers.hlsli"
#include "./../../../../assets/Shaders/HLSL/Shadow.hlsli"

ConstantBuffer<ConstantBuffers::LightCB> cbLight : register(b0);

Shadow_DrawShadow_RootConstants(b1)

Texture2D<ShadingConvention::GBuffer::PositionMapFormat>   gi_PositionMap   : register(t0);
Texture2D<ShadingConvention::Shadow::ZDepthMapFormat>      gi_ZDepthMap     : register(t1);
Texture2DArray<ShadingConvention::Shadow::ZDepthMapFormat> gi_ZDepthCubeMap : register(t2);

RWTexture2D<ShadingConvention::Shadow::ShadowMapFormat>    guio_ShadowMap   : register(u0);

[numthreads(
    ShadingConvention::Shadow::ThreadGroup::DrawShadow::Width, 
    ShadingConvention::Shadow::ThreadGroup::DrawShadow::Height, 
    ShadingConvention::Shadow::ThreadGroup::DrawShadow::Depth)]
void CS(in uint2 DTid : SV_DispatchThreadID) {
    uint value = gLightIndex != 0 ? guio_ShadowMap[DTid] : 0;

    Render::DX::Foundation::Light light = cbLight.Lights[gLightIndex];

    const float4 PosW = gi_PositionMap.Load(uint3(DTid, 0));
    if (!ShadingConvention::GBuffer::IsValidPosition(PosW)) {
        guio_ShadowMap[DTid] = value;
        return;
    }
    
    if (light.Type == Common::Render::LightType::E_Directional) {
        const float ShadowFactor = Shadow::CalcShadowFactorDirectional(gi_ZDepthMap, gsamShadow, light.Mat1, PosW.xyz);
        value = Shadow::CalcShiftedShadowValueF(ShadowFactor, value, gLightIndex);
    }
    else if (light.Type == Common::Render::LightType::E_Point) {
        const float3 Direction = PosW.xyz - light.Position;
        const uint Index = ShaderUtil::GetCubeFaceIndex(Direction);
        const float3 Normalized = normalize(Direction);
        const float2 UV = ShaderUtil::ConvertDirectionToUV(Normalized);

        const float4x4 ViewProj = Shadow::GetViewProjMatrix(light, Index);
        const float ShadowFactor = Shadow::CalcShadowFactorCubeCS(gi_ZDepthCubeMap, gsamShadow, ViewProj, PosW.xyz, UV, Index);

        value = Shadow::CalcShiftedShadowValueF(ShadowFactor, value, gLightIndex);
    }
    //else {
    //    const float shadowFactor = CalcShadowFactorDirectional(gi_ZDepthMap, gsamShadow, light.Mat0, posW.xyz);
    //    value = CalcShiftedShadowValueF(shadowFactor, value, gLightIndex);
    //}
	
    guio_ShadowMap[DTid] = value;
}

#endif // __DRAWSHADOW_HLSL__