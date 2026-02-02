#ifndef __SHADOW_HLSLI__
#define __SHADOW_HLSLI__

namespace Shadow {    
    float4x4 GetViewProjMatrix(in Common::Foundation::Light light, in uint face) {
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
    
    float CalcShadowFactor(
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
    
    float CalcShadowFactorCube(
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
    
    uint GetShiftedShadowValue(in uint value, in uint index) {
        return (value >> index) & 1;
    }
    
    void CalcShadowFactorsPCF5x5(
            in Texture2D<uint> shadowMap, in uint2 dims, in float2 texc, in uint numLights,
            out float shadowFactors[MaxLights]) {    
        if (numLights == 0) return;
            
        float sum[MaxLights];
        [unroll]
        for (uint i = 0; i < MaxLights; ++i) sum[i] = 0.f;
    
        const float2 Pixel = texc * dims;
        const int2 BasePixel = (int2)floor(Pixel + 0.5f);
    
        const int Radius = 2;
        const int Diameter = (2 * Radius) + 1;
        const float invCount = 1.0f / (Diameter * Diameter);
    
        [loop]
        for (int dy = -Radius; dy <= Radius; ++dy) {
            [loop]
            for (int dx = -Radius; dx <= Radius; ++dx) {
                int2 tap = BasePixel + int2(dx, dy);
                tap = clamp(tap, int2(0, 0), int2((int)dims.x - 1, (int)dims.y - 1));
    
                const uint Value = shadowMap.Load(uint3((uint2)tap, 0));
    
                [loop]
                for (uint i = 0; i < numLights; ++i)
                    sum[i] += Shadow::GetShiftedShadowValue(Value, i);
            }
        }
    
        [loop]
        for (uint i = 0; i < numLights; ++i)
            shadowFactors[i] = sum[i] * invCount;
    }
        
    void CalcShadowFactors(
            in Texture2D<uint> shadowMap, in uint2 dims, in float2 texc, in uint numLights,
            out float shadowFactors[MaxLights]) {        
        const uint2 Index = min((uint2)(texc * dims + 0.5f), dims - 1);                                                                                                                                                                                                                                                                                            
        const uint Value = shadowMap.Load(uint3(Index, 0));
        
    	[loop]
        for (uint i = 0; i < numLights; ++i) 
            shadowFactors[i] = GetShiftedShadowValue(Value, i);
    }
}

#endif // __SHADOW_HLSLI__