#ifndef __DRAWSHADOW_HLSL__
#define __DRAWSHADOW_HLSL__

#ifndef _HLSL
#define _HLSL
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
    uint value = _Light.Index != 0 ? gio_ShadowMap[DTid] : 0;
    
    float4 posW = gi_PositionMap[DTid]; 
    if (posW.a == -1.f) {
        gio_ShadowMap[DTid] = value;
        return;
    }
    
    if (_Light.Type == LightType_Directional || LightType_Spot) {
        float shadowFactor = Shadow::CalcShadowFactor(
            gi_ZDepthMap, gsamShadow, _Light.Mat1, posW.xyz);
        value = Shadow::CalcShiftedShadowValueF(shadowFactor, value, _Light.Index);
    }
    else if (_Light.Type == LightType_Point) {
        
    }
    
    gio_ShadowMap[DTid] = value;
}

#endif // __DRAWSHADOW_HLSL__