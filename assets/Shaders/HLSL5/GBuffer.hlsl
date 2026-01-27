#ifndef __GBUFFER_HLSL__
#define __GBUFFER_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../../inc/Render/DX11/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL5/ShaderUtil.hlsli"
#include "./../../../assets/Shaders/HLSL5/GBuffer.hlsli"

PassCB_register(b0);
ObjectCB_register(b1);
MaterialCB_register(b2);
GBufferCB_register(b3);

VERTEX_IN

struct VertexOut {
    float4 PosH         : SV_Position;
    float4 CurrPosH     : POSITION0;
    float4 PrevPosH     : POSITION1;
    float3 PosW         : POSITION2;
    float3 NormalW      : NORMAL0;
    float3 PrevNormalW  : NORMAL1;
    float2 TexC         : TEXCOORD0;
};

struct PixelOut {
    float4 Diffuse            : SV_Target0;
    float4 Normal             : SV_Target1;
    float4 Position           : SV_Target2;
    float2 RoughnessMetalness : SV_Target3;
    float2 Velocity           : SV_Target4;
};

VertexOut VS(in VertexIn vin) {
    VertexOut vout = (VertexOut)0;
    
    vout.TexC = vin.TexC;
    
    vout.NormalW = mul(vin.NormalL, (float3x3)World);
    vout.PrevNormalW = mul(vin.NormalL, (float3x3)PrevWorld);
    
    float4 posW = mul(float4(vin.PosL, 1.f), World);
    vout.PosW = posW.xyz;
    
    float4 posH = mul(posW, ViewProj);
    vout.CurrPosH = posH;
    vout.PosH = posH + float4(JitteredOffset * posH.w, 0.f, 0.f);
    
    float4 prevPosW = mul(float4(vin.PosL, 1), PrevWorld);
    vout.PrevPosH = mul(prevPosW, PrevViewProj);
    
    return vout;
}

PixelOut PS(VertexOut pin) : SV_Target {    
    pin.CurrPosH /= pin.CurrPosH.w;
    
    const float2 UV = pin.CurrPosH.xy * 0.5f + 0.5f;
    const uint2 ScreenPos = (uint2)floor(UV * TexDim);
    const uint2 ScreenPos_xN = ScreenPos >> 1;
    const float Threshold = gThresholdMatrix8x8[ScreenPos_xN.y & 7][ScreenPos_xN.x & 7];
    
    float4 posV = mul(pin.CurrPosH, InvProj);
	posV /= posV.w;

	const float Dist = (posV.z - DitheringMinDist) / (DitheringMaxDist - DitheringMinDist);

	clip(Dist - Threshold);
    
    PixelOut pout = (PixelOut)0;
    
    pin.PrevPosH /= pin.PrevPosH.w;
    float2 velocity = ShaderUtil::CalcVelocity(pin.CurrPosH, pin.PrevPosH);
    
    pout.Diffuse = Albedo;
    pout.Normal = float4(normalize(pin.NormalW), 0.f);
    pout.Position = float4(pin.PosW, 1.f);
    pout.RoughnessMetalness = float2(Roughness, Metalness);
    pout.Velocity = velocity;
    
    return pout;
}

#endif // __GBUFFER_HLSL__