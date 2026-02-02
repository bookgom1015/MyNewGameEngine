#ifndef __DRAWSHADOW_HLSL__
#define __DRAWSHADOW_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef HLSL_VERSION_UNDER_6
#define HLSL_VERSION_UNDER_6
#endif

#include "./../../../inc/Render/DX11/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"
#include "./../../../assets/Shaders/HLSL/Shadow.hlsli"

LightCB_register(b0);
ShadowCB_register(b1);

Texture2D<float4>     gi_PositionMap    : register(t0);
Texture2D<float>      gi_ZDepthMap      : register(t1);
Texture2DArray<float> gi_ZDepthCubeMap  : register(t2);

RWTexture2D<uint> gio_ShadowMap : register(u0);

[numthreads(8, 8, 1)]
void CS(in uint2 DTid : SV_DispatchThreadID) {    
    uint value = LightIndex != 0 ? gio_ShadowMap[DTid] : 0;
    
    Common::Foundation::Light light = Lights[LightIndex];
    
    float4 posW = gi_PositionMap[DTid]; 
    if (posW.a == -1.f) {
        gio_ShadowMap[DTid] = value;
        return;
    }
    
    if (light.Type == Common::Foundation::LightType_Directional || light.Type == Common::Foundation::LightType_Spot) {
        float shadowFactor = Shadow::CalcShadowFactor(
            gi_ZDepthMap, gsamShadow, light.Mat1, posW.xyz);
        value = Shadow::CalcShiftedShadowValueF(shadowFactor, value, LightIndex);
    }
    else if (light.Type == Common::Foundation::LightType_Point) {
        float3 direction;
        if (light.Type == Common::Foundation::LightType_Tube) {
            direction = posW.xyz - (light.Position + light.Position1) * 0.5f;
        }
        else {
            direction = posW.xyz - light.Position;
        }
        
        uint index = ShaderUtil::GetCubeFaceIndex(direction);
        float3 normalized = normalize(direction);
        
        float2 uv = ShaderUtil::ConvertDirectionToUV(normalized);

        const float4x4 ViewProj = Shadow::GetViewProjMatrix(light, index);
        const float ShadowFactor = Shadow::CalcShadowFactorCube(
            gi_ZDepthCubeMap, gsamShadow, ViewProj, posW.xyz, uv, index);

        value = Shadow::CalcShiftedShadowValueF(ShadowFactor, value, LightIndex);
    }
    
    gio_ShadowMap[DTid] = value;
}

#endif // __DRAWSHADOW_HLSL__