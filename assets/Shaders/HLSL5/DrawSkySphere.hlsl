#ifndef __DRAWSKYSPHERE_HLSL__
#define __DRAWSKYSPHERE_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef HLSL_VERSION_UNDER_6
#define HLSL_VERSION_UNDER_6
#endif

#include "./../../../inc/Render/DX11/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL5/Samplers.hlsli"

PassCB_register(b0);

TextureCube<float4> gi_EnvironmentCubeMap : register(t0);

VERTEX_IN

struct VertexOut {
    float4 PosH : SV_Position;
    float3 PosL : POSITION0;
    float4 Clip : TEXCOORD0;
};

VertexOut VS(in VertexIn vin) {
    VertexOut vout = (VertexOut)0;
    
    vout.PosL = vin.PosL;
    
    float4 PosW = float4(vin.PosL, 1.f);
    PosW.xyz += EyePosW;
    
    float4 clip = mul(PosW, ViewProj);
    vout.PosH = clip.xyww;
    vout.Clip = clip;
    
    return vout;
}

float4 PS(in VertexOut pin) : SV_Target {
    return gi_EnvironmentCubeMap.SampleLevel(gsamLinearClamp, normalize(pin.PosL), 0);
    
    //float3 upperColor = float3(20.f / 255.f, 204.f / 255.f, 230.f / 255.f);
    //float3 lowerColor = float3(101.f / 255.f, 214.f / 255.f, 172.f / 255.f);
    //
    //float4 ndc = pin.Clip / pin.Clip.w;
    //float2 texc = ndc.xy * 0.5f + 0.5f;
    //
    //float3 resultColor = lerp(lowerColor, upperColor, texc.y);
    //
    //return float4(resultColor, 1.f);
}

#endif // __DRAWSKYSPHERE_HLSL__