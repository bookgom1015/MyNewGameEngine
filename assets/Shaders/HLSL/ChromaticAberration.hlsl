#ifndef __CHROMATICABERRATION_HLSL__
#define __CHROMATICABERRATION_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef _FIT_TO_SCREEN_COORD
#define _FIT_TO_SCREEN_COORD
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"

ChromaticAberration_Default_RootConstants(b0)

Texture2D<HDR_FORMAT> gi_BackBuffer : register(t0);

FitToScreenVertexOut

FitToScreenVertexShader

FitToScreenMeshShader

HDR_FORMAT PS(in VertexOut pin) : SV_Target {
    // Centered Coords
    const float2 Delta = pin.TexC - 0.5f;
    const float Radius = length(Delta);
    const float Clamped = saturate(Radius / 0.70710678f);

    // Threshold + featherd mask: 0 inside, at 1 outside
    const float Start = saturate(gThreshold);
    const float End = saturate(gThreshold + max(gFeather, 1e-4f));
    
    float mask = smoothstep(Start, End, Clamped);
    // Non-linear ramp (more control)
    mask = pow(mask, max(gExponent, 1e-4f));

    // Direction: radial outward
    const float2 Direction = (Radius > 1e-6f) ? (Delta / Radius) : (float2)0;

    // Shift amount in uv (convert pixels to uv)
    const float ShiftPixel = gMaxShiftPx * gStrength * mask;
    const float2 ShiftTexC = ShiftPixel * gInvTexDim * Direction;

    // Sampler per channel
    const float3 Scene = gi_BackBuffer.Sample(gsamLinearClamp, pin.TexC).rgb;
    
    const float rC = gi_BackBuffer.Sample(gsamLinearClamp, pin.TexC + ShiftTexC).r; // Outward
    const float gC = Scene.g;                                                       // Stable
    const float bC = gi_BackBuffer.Sample(gsamLinearClamp, pin.TexC - ShiftTexC).b; // Inward

    return float4(rC, gC, bC, 1.f);
}

#endif // __CHROMATICABERRATION_HLSL__