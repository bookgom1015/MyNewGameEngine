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

FitToScreenVertexOut

FitToScreenVertexShader

FitToScreenMeshShader

HDR_FORMAT PS(in VertexOut pin) : SV_TARGET {    
    const float2 Velocity = gi_VelocityMap.Sample(gsamLinearClamp, pin.TexC);
    const float3 BackBufferColor = gi_BackBuffer.SampleLevel(gsamLinearClamp, pin.TexC, 0).rgb;
    
    if (!ShadingConvention::GBuffer::IsValidVelocity(Velocity)) return float4(BackBufferColor, 1.f);
    
    const float2 PrevTexC = pin.TexC - Velocity;
    if (any(PrevTexC < 0.f) || any(PrevTexC > 1.f)) return float4(BackBufferColor, 1.f);

    float3 historyColor = gi_HistoryMap.SampleLevel(gsamPointClamp, PrevTexC, 0).rgb;
    const float3 Unclamped = historyColor;
        
    float3 minC = BackBufferColor;
    float3 maxC = BackBufferColor;
    [unroll] for(int oy = -1; oy <= 1; ++oy)
    [unroll] for(int ox = -1; ox <= 1; ++ox) {
      const float2 uv = pin.TexC + float2(ox, oy) * gInvTexDim;
      const float3 c = gi_BackBuffer.SampleLevel(gsamPointClamp, saturate(uv), 0).rgb;
      minC = min(minC, c);
      maxC = max(maxC, c);
    }
    historyColor = clamp(historyColor, minC, maxC);

    const float v = length(Velocity); 
    float w = gModulationFactor;                                                                                                                                                                                                                                                                                            
    w *= exp2(-v * 2.f);
                                                                                                                                                                                                                                                                                            
    const float ClipAmt = length(Unclamped - historyColor);
    w *= exp2(-ClipAmt * 4.f);
                                                                                                                                                                                                                                                                                            
    const float3 ResolvedColor = lerp(BackBufferColor, historyColor, w);

    return float4(ResolvedColor, 1.f);
}

#endif // __TAA_HLSL__