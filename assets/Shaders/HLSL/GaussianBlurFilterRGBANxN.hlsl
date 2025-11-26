#ifndef __GAUSSIANBLURFILTERRGBANXN_HLSL__
#define __GAUSSIANBLURFILTERRGBANXN_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef _FIT_TO_SCREEN_COORD
#define _FIT_TO_SCREEN_COORD
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"

BlurFilter_Default_RootConstants(b0)

// Kernel Radius: 1 => 3x3, 2 => 5x5, 3 => 7x7, 4 => 9x9
#ifndef KERNEL_RADIUS
#define KERNEL_RADIUS 1
#endif

static const int KERNEL_SIZE = (KERNEL_RADIUS * 2 + 1);

static const float GAUSS_SIGMA =
    (KERNEL_RADIUS == 1) ? 1.0f :   // 3x3
    (KERNEL_RADIUS == 2) ? 1.2f :   // 5x5
    (KERNEL_RADIUS == 3) ? 1.8f :   // 7x7
                           2.0f;    // 9x9 (KERNEL_RADIUS == 4)

Texture2D<HDR_FORMAT>   gi_InputMap  : register(t0);

FitToScreenVertexOut

FitToScreenVertexShader

FitToScreenMeshShader

void AddFilterContribution(
        inout HDR_FORMAT weightedValueSum,
        inout float weightSum,
        in int2 baseId,
        in int2 offset) {
    const int2 id = baseId + offset;

    if (id.x >= 0 && id.y >= 0 && id.x < gTexDim.x && id.y < gTexDim.y) {
        const float2 d = float2(offset);
        const float sigma2 = GAUSS_SIGMA * GAUSS_SIGMA;

        // 2D Gaussian: exp(-(x^2 + y^2) / (2sig^2))
        const float weight = exp(-dot(d, d) / (2.f * sigma2));

        weightedValueSum += weight * gi_InputMap.Load(int3(id, 0));
        weightSum += weight;
    }
}

HDR_FORMAT PS(in VertexOut pin) : SV_Target {
    //return (float4)1.f;
    uint2 size;
    gi_InputMap.GetDimensions(size.x, size.y);
    
    const int2 id = size * pin.TexC - 0.5f;

    float weightSum = 0.f;
    HDR_FORMAT weightedValueSum = 0.f;

    [unroll]
    for (int y = -KERNEL_RADIUS; y <= KERNEL_RADIUS; ++y) {
        [unroll]
        for (int x = -KERNEL_RADIUS; x <= KERNEL_RADIUS; ++x) {
            AddFilterContribution(weightedValueSum, weightSum, id, int2(x, y));
        }
    }
    
    const HDR_FORMAT center = gi_InputMap.Load(int3(id, 0));
    HDR_FORMAT result = (weightSum > 0.f) ? (weightedValueSum / weightSum) : center;

    return result;
}

#endif // __GAUSSIANBLURFILTERRGBANXN_HLSL__