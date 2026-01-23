#ifndef __GBUFFER_HLSL__
#define __GBUFFER_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../../inc/Render/DX11/Foundation/HlslCompaction.h"

PassCB_register(b0);
ObjectCB_register(b1);
MaterialCB_register(b2);

VERTEX_IN

struct VertexOut {
    float4 PosH     : SV_Position;
    float3 PosW     : POSITION0;
    float3 NormalW  : NORMAL0;
    float2 TexC     : TEXCOORD0;
};

struct PixelOut {
    float4 Diffuse            : SV_Target0;
    float4 Normal             : SV_Target1;
    float4 Position           : SV_Target2;
    float2 RoughnessMetalness : SV_Target3;
};

VertexOut VS(in VertexIn vin) {
    VertexOut vout = (VertexOut)0;
    
    vout.TexC = vin.TexC;
    
    vout.NormalW = mul(vin.NormalL, (float3x3)World);
    
    float4 posW = mul(float4(vin.PosL, 1.f), World);
    vout.PosW = posW.xyz;
    
    float4 posV = mul(posW, View);
    float4 posH = mul(posV, Proj);
    vout.PosH = posH;// + float4(JitteredOffset * posH.w, 0.f, 0.f);
    
    return vout;
}

PixelOut PS(VertexOut pin) : SV_Target {
    PixelOut pout = (PixelOut)0;
    
    pout.Diffuse = Albedo;
    pout.Normal = float4(normalize(pin.NormalW), 0.f);
    pout.Position = float4(pin.PosW, 1.f);
    pout.RoughnessMetalness = float2(Roughness, Metalness);
    
    return pout;
}

#endif // __GBUFFER_HLSL__