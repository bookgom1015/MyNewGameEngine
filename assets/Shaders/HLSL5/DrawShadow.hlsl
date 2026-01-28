#ifndef __DRAWSHADOW_HLSL__
#define __DRAWSHADOW_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef HLSL_VERSION_UNDER_6
#define HLSL_VERSION_UNDER_6
#endif

#include "./../../../inc/Render/DX11/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL5/Samplers.hlsli"
#include "./../../../assets/Shaders/HLSL5/Shadow.hlsli"

LightCB_register(b0);

Texture2D<float4> gi_PositionMap    : register(t0);
Texture2D<float>  gi_ZDepthMap      : register(t1);
Texture2D<float>  gi_ZDepthCubeMap  : register(t2);

RWTexture2D<uint> gio_ShadowMap : register(u0);

[numthreads(8, 8, 1)]
void CS(in uint2 DTid : SV_DispatchThreadID) {    
    uint value = Light.Index != 0 ? gio_ShadowMap[DTid] : 0;
    
    float4 posW = gi_PositionMap[DTid]; 
    if (posW.a == -1.f) {
        gio_ShadowMap[DTid] = value;
        return;
    }
    
    if (Light.Type == Common::Foundation::LightType_Directional || Light.Type == Common::Foundation::LightType_Spot) {
        float shadowFactor = Shadow::CalcShadowFactor(
            gi_ZDepthMap, gsamShadow, Light.Mat1, posW.xyz);
        value = Shadow::CalcShiftedShadowValueF(shadowFactor, value, Light.Index);
    }
    else if (Light.Type == Common::Foundation::LightType_Point) {
        
    }
    
    gio_ShadowMap[DTid] = value;
}

#endif // __DRAWSHADOW_HLSL__