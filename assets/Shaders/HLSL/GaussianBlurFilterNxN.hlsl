#ifndef __GAUSSIANBLURFILTERNXN_HLSL__
#define __GAUSSIANBLURFILTERNXN_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef ValueType
#define ValueType float
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

Texture2D<ValueType>   gi_InputMap  : register(t0);
RWTexture2D<ValueType> go_OutputMap : register(u0);

void AddFilterContribution(
        inout ValueType weightedValueSum,
        inout float weightSum,
        in int2 baseId,
        in int2 offset) {
    const int2 id = baseId + offset;

    if (id.x >= 0 && id.y >= 0 && id.x < gTexDim.x && id.y < gTexDim.y) {
        const float2 d = float2(offset);
        const float sigma2 = GAUSS_SIGMA * GAUSS_SIGMA;

        // 2D Gaussian: exp(-(x^2 + y^2) / (2sig^2))
        const float weight = exp(-dot(d, d) / (2.f * sigma2));

        weightedValueSum += weight * gi_InputMap[id];
        weightSum += weight;
    }
}

[numthreads(
    ShadingConvention::BlurFilter::ThreadGroup::Default::Width,
    ShadingConvention::BlurFilter::ThreadGroup::Default::Height,
    ShadingConvention::BlurFilter::ThreadGroup::Default::Depth)]
void CS(in uint2 DTid : SV_DispatchThreadID) {
    float weightSum = 0.f;
    ValueType weightedValueSum = 0.f;

    [unroll]
    for (int y = -KERNEL_RADIUS; y <= KERNEL_RADIUS; ++y) {
        [unroll]
        for (int x = -KERNEL_RADIUS; x <= KERNEL_RADIUS; ++x) {
            AddFilterContribution(weightedValueSum, weightSum, int2(DTid), int2(x, y));
        }
    }
    
    const ValueType center = gi_InputMap[DTid];
    go_OutputMap[DTid] = (weightSum > 0.f) ? (weightedValueSum / weightSum) : center;
}

#endif // __GAUSSIANBLURFILTERNXN_HLSL__