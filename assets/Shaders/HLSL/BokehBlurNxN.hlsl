#ifndef __BOKEHBLURNXN_HLSL__
#define __BOKEHBLURNXN_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef _FIT_TO_SCREEN_COORD
#define _FIT_TO_SCREEN_COORD
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"

DOF_BokehBlur3x3_RootConstants(b0)

FitToScreenVertexOut

FitToScreenVertexShader

FitToScreenMeshShader
                                                                                                                                                                                                                                                                
// Kernel Radius: 1 => 3x3, 2 => 5x5, 3 => 7x7, 4 => 9x9
#ifndef KERNEL_RADIUS
#define KERNEL_RADIUS 4
#endif

static const int KERNEL_SIZE = (KERNEL_RADIUS * 2 + 1);

static const float GAUSS_SIGMA =
    (KERNEL_RADIUS == 1) ? 0.5f : // 3x3
    (KERNEL_RADIUS == 2) ? 1.f  : // 5x5
    (KERNEL_RADIUS == 3) ? 1.5f : // 7x7
                           2.f;   // 9x9 (KERNEL_RADIUS == 4)

Texture2D<HDR_FORMAT> gi_InputMap  : register(t0);
Texture2D<ShadingConvention::DOF::CircleOfConfusionMapFormat> gi_CoCMap : register(t1);

void AddFilterContribution(
        inout float3 weightedValueSum,
        inout float weightSum,
        in float2 uv,
        in float2 ddxy,
        in int2 offset) {
    const float2 TexC = uv + ddxy * offset;

    if (all(TexC >= 0.f) && all(TexC <= gTexDim)) {
        const float2 d = float2(offset);
        const float twoSigma2 = 2 * GAUSS_SIGMA * GAUSS_SIGMA;

        // 2D Gaussian: exp(-(x^2 + y^2) / (2sig^2))
        const float weight = exp(-dot(d, d) / (twoSigma2));

        weightedValueSum += weight * gi_InputMap.SampleLevel(gsamLinearClamp, TexC, 0).rgb;
        weightSum += weight;
    }
}

HDR_FORMAT PS(VertexOut pin) : SV_TARGET {
    float weightSum = 0.f;
    float3 weightedValueSum = (float3)0;
    
    const float3 Center = gi_InputMap.SampleLevel(gsamLinearClamp, pin.TexC, 0).rgb;
    
    const float CoC = gi_CoCMap.SampleLevel(gsamLinearClamp, pin.TexC, 0);
    if (abs(CoC) < 0.08f) return float4(Center, 1.f);

    [unroll]
    for (int y = -3; y <= 3; ++y) {
        [unroll]
        for (int x = -3; x <= 3; ++x) {
            AddFilterContribution(
                weightedValueSum, weightSum, pin.TexC, gInvTexDim, int2(x, y));
        }
    }
    
    return float4((weightSum > 0.f) ? (weightedValueSum / weightSum) : Center, 1.f);
}

#endif // __BOKEHBLURNXN_HLSL__