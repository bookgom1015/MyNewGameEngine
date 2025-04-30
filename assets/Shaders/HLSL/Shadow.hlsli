#ifndef __SHADOW_HLSLI__
#define __SHADOW_HLSLI__

namespace Shadow {
    float4x4 GetViewProjMatrix(in Render::DX::Foundation::Light light, in uint face) {
        switch (face) {
            case 0: return light.Mat0;
            case 1: return light.Mat1;
            case 2: return light.Mat2;
            case 3: return light.Mat3;
            case 4: return light.Mat4;
            case 5: return light.Mat5;
            default: return (float4x4)0.f;
        }
    }
    
    float CalcShadowFactorDirectional(
		    in Texture2D<float> depthMap,
		    in SamplerComparisonState sampComp,
		    in float4x4 viewProjTex,
		    in float3 fragPosW) {
        float4 shadowPosH = mul(float4(fragPosW, 1.f), viewProjTex);
        shadowPosH /= shadowPosH.w;

        const float Depth = shadowPosH.z;

        return depthMap.SampleCmpLevelZero(sampComp, shadowPosH.xy, Depth);
    }
    
    uint CalcShiftedShadowValueF(in float percent, in uint value, in uint index) {
        const uint ShadowFactor = percent < 0.5f ? 0 : 1;
        const uint Shifted = ShadowFactor << index;

        return value | Shifted;
    }
    
    float CalcShadowFactorCubeCS(
		    in Texture2DArray<float> depthMap,
		    in SamplerComparisonState sampComp,
		    in float4x4 viewProj,
		    in float3 fragPosW,
		    in float2 uv,
		    in uint index) {
        float4 shadowPosH = mul(float4(fragPosW, 1.f), viewProj);
        shadowPosH /= shadowPosH.w;

        const float Depth = shadowPosH.z;

        return depthMap.SampleCmpLevelZero(sampComp, float3(uv, index), Depth);
    }
}

#endif // __SHADOW_HLSLI__