#ifndef __SHADOW_HLSLI__
#define __SHADOW_HLSLI__

namespace Shadow {
    uint CalcShiftedShadowValueF(in float percent, in uint value, in uint index) {
        uint shadowFactor = percent < 0.5f ? 0 : 1;
        uint shifted = shadowFactor << index;
    
        return value | shifted;
    }
    
    uint GetShiftedShadowValue(in uint value, in uint index) {
        return (value >> index) & 1;
    }
    
    float CalcShadowFactor(
            in Texture2D<float> depthMap,
            in SamplerComparisonState sampComp,
            in float4x4 viewProjTex,
            in float3 fragPosW) {
        float4 shadowPosH = mul(float4(fragPosW, 1.f), viewProjTex);
        shadowPosH /= shadowPosH.w;
    
        float depth = shadowPosH.z;
    
        return depthMap.SampleCmpLevelZero(sampComp, shadowPosH.xy, depth);
    }
}

#endif // __SHADOW_HLSLI__