#ifndef __BOKEHBLUR_HLSL__
#define __BOKEHBLUR_HLSL__

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

static const int KERNEL_SIZE = 11;
static const float GAUSS_SIGMA = 1.2f;

Texture2D<HDR_FORMAT> gi_InputMap  : register(t0);
Texture2D<ShadingConvention::DOF::CircleOfConfusionMapFormat> gi_CoCMap : register(t1);

void AddFilterContribution(
        inout float4 weightedValueSum,
        inout float weightSum,
        in int2 baseId,
        in int2 offset) {
    const int2 id = baseId + offset;  
    const float CenterCoC = abs(gi_CoCMap[baseId]);
    if (id.x >= 0 && id.y >= 0 && id.x < gTexDim.x && id.y < gTexDim.y && CenterCoC > 0.1f) {        
        const float2 d = float2(offset);
        const float sigma2 = GAUSS_SIGMA * GAUSS_SIGMA;
        
        const float NeighborCoC = abs(gi_CoCMap[id]);
        
        // 2D Gaussian: exp(-(x^2 + y^2) / (2sig^2))
        const float weight = exp(-dot(d, d) / (2.f * sigma2)) * CenterCoC * NeighborCoC;

        weightedValueSum += weight * gi_InputMap[id];
        weightSum += weight;
    }
}

HDR_FORMAT PS(VertexOut pin) : SV_TARGET {
    float weightSum = 0.f;
    float4 weightedValueSum = (float4)0;
    
    const uint2 DTid = (pin.TexC * gTexDim) - 0.5f;

    [unroll]
    for (int y = -3; y <= 3; ++y) {
        [unroll]
        for (int x = -3; x <= 3; ++x) {
            AddFilterContribution(weightedValueSum, weightSum, int2(DTid), int2(x, y));
        }
    }
    
    const float4 center = gi_InputMap[DTid];
    return (weightSum > 0.f) ? (weightedValueSum / weightSum) : center;
}

#endif // __BOKEHBLUR_HLSL__