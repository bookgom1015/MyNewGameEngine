#ifndef __TAA_HLSL__
#define __TAA_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef _FIT_TO_SCREEN_COORD
#define _FIT_TO_SCREEN_COORD
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"

TAA_Default_RootConstants(b0)

Texture2D<HDR_FORMAT>                                    gi_BackBuffer  : register(t0);
Texture2D<HDR_FORMAT>                                    gi_HistoryMap  : register(t1);
Texture2D<ShadingConvention::GBuffer::VelocityMapFormat> gi_VelocityMap : register(t2);

struct VertexOut {
    float4 PosH : SV_POSITION;
    float2 TexC : TEXCOORD;
};

FitToScreenVertexShader

FitToScreenMeshShader

HDR_FORMAT PS(in VertexOut pin) : SV_TARGET {    
    const float2 Velocity = gi_VelocityMap.Sample(gsamPointClamp, pin.TexC);
    const float3 BackBufferColor = gi_BackBuffer.SampleLevel(gsamPointClamp, pin.TexC, 0).rgb;
    
    if (!ShadingConvention::GBuffer::IsValidVelocity(Velocity)) return float4(BackBufferColor, 1.f);
    
    const float2 PrevTexC = pin.TexC - Velocity;

    float3 historyColor = gi_HistoryMap.SampleLevel(gsamLinearClamp, PrevTexC, 0).rgb;
        
    const float2 dx = float2(gInvTexDim.x, 0.f);
    const float2 dy = float2(0.f, gInvTexDim.y);

    const float3 NearColor0 = gi_BackBuffer.SampleLevel(gsamPointClamp, saturate(pin.TexC + dx), 0).rgb;
    const float3 NearColor1 = gi_BackBuffer.SampleLevel(gsamPointClamp, saturate(pin.TexC - dx), 0).rgb;
    const float3 NearColor2 = gi_BackBuffer.SampleLevel(gsamPointClamp, saturate(pin.TexC + dy), 0).rgb;
    const float3 NearColor3 = gi_BackBuffer.SampleLevel(gsamPointClamp, saturate(pin.TexC - dy), 0).rgb;

    const float3 MinColor = min(BackBufferColor, min(NearColor0, min(NearColor1, min(NearColor2, NearColor3))));
    const float3 MaxColor = max(BackBufferColor, max(NearColor0, max(NearColor1, max(NearColor2, NearColor3))));

    historyColor = clamp(historyColor, MinColor, MaxColor);

    const float3 ResolvedColor = BackBufferColor * (1.f - gModulationFactor) + historyColor * gModulationFactor;

    return float4(ResolvedColor, 1.f);
}

#endif // __TAA_HLSL__