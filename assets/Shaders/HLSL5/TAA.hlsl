#ifndef __TAA_HLSL__
#define __TAA_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef HLSL_VERSION_UNDER_6
#define HLSL_VERSION_UNDER_6
#endif

#ifndef _FIT_TO_SCREEN_COORD
#define _FIT_TO_SCREEN_COORD
#endif

#include "./../../../inc/Render/DX11/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"

Texture2D<float4> gi_BackBuffer  : register(t0);
Texture2D<float4> gi_HistoryMap  : register(t1);
Texture2D<float2> gi_VelocityMap : register(t2);

FitToScreenVertexOut

FitToScreenVertexShader

float4 PS(in VertexOut pin) : SV_TARGET {        
    float2 velocity = gi_VelocityMap.Sample(gsamLinearClamp, pin.TexC);
    float3 scene = gi_BackBuffer.SampleLevel(gsamLinearClamp, pin.TexC, 0).rgb;
    
    if (all(velocity == 1000.f)) return float4(scene, 1.f);
    
    float2 prevTexC = pin.TexC - velocity;
    if (any(prevTexC < 0.f) || any(prevTexC > 1.f)) return float4(scene, 1.f);

    float3 historyColor = gi_HistoryMap.SampleLevel(gsamPointClamp, prevTexC, 0).rgb;
    float3 unclamped = historyColor;
    
    uint2 size;
    gi_BackBuffer.GetDimensions(size.x, size.y);
    
    float2 ddxy = 1.f / size;
            
    float3 minC = scene;
    float3 maxC = scene;
    [unroll] for(int oy = -1; oy <= 1; ++oy)
    [unroll] for(int ox = -1; ox <= 1; ++ox) {
      const float2 uv = pin.TexC + float2(ox, oy) * ddxy;
      const float3 c = gi_BackBuffer.SampleLevel(gsamPointClamp, saturate(uv), 0).rgb;
      minC = min(minC, c);
      maxC = max(maxC, c);
    }
    historyColor = clamp(historyColor, minC, maxC);

    const float v = length(velocity); 
    float w = 0.8f;                                                                                                                                                                                                                                                                                            
    w *= exp2(-v * 2.f);
                                                                                                                                                                                                                                                                                            
    const float ClipAmt = length(unclamped - historyColor);
    w *= exp2(-ClipAmt * 4.f);
                                                                                                                                                                                                                                                                                            
    const float3 ResolvedColor = lerp(scene, historyColor, w);

    return float4(ResolvedColor, 1.f);
}

#endif // __TAA_HLSL__