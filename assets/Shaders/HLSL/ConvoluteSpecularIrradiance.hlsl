#ifndef __CONVOLUTESPECULARIRRADIANCE_HLSL__
#define __CONVOLUTESPECULARIRRADIANCE_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef _CUBE_COORD
#define _CUBE_COORD
#endif 

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"
#include "./../../../assets/Shaders/HLSL/BRDF.hlsli"

ConstantBuffer<ConstantBuffers:: ProjectToCubeCB> cbProjectToCube : register(b0);

EnvironmentMap_ConvoluteSpecularIrradiance_RootConstants(b1)

TextureCube<HDR_FORMAT> gi_EnvCubeMap : register(t0);

struct VertexOut {
    float3 PosL : POSITION;
};

struct GeoOut {
    float4 PosH : SV_Position;
    float3 PosL : POSITION;
    uint ArrayIndex : SV_RenderTargetArrayIndex;
};

static const int SAMPLE_COUNT = 8192;

FitToCubeVertexShader

FitToCubeGeometryShader

float ChetanJagsMipLevel(in float3 N, in float3 V, in float3 H, in float roughness) {
    const float D = DistributionGGX(N, H, roughness);
    const float NdotH = max(dot(N, H), 0.f);
    const float HdotV = max(dot(H, V), 0.f);
    const float PDF = D * NdotH / (4.f * HdotV) + 0.0001f;

    const float Resolution2 = gResolution * gResolution;
    const float saTexel = 4.f * PI / (6.f * Resolution2);
    const float saSample = 1.f / ((float) SAMPLE_COUNT * PDF + 0.0001f);

    const float MipLevel = roughness == 0.f ? 0.f : 0.5f * log2(saSample / saTexel);
    
    return MipLevel;
}

HDR_FORMAT PS(in GeoOut pin) : SV_Target {
    const float3 N = normalize(pin.PosL);
    const float3 R = N;
    const float3 V = R;

    float totalWeight = 0.f;
    float3 prefilteredColor = 0.f;

    for (uint i = 0; i < SAMPLE_COUNT; ++i) {
        const float2 Xi = Hammersley(i, SAMPLE_COUNT);
        const float3 H = ImportanceSampleGGX(Xi, N, gRoughness);
        const float3 L = normalize(2.f * dot(V, H) * H - V);
    
        const float NdotL = max(dot(N, L), 0.f);
        if (NdotL > 0.f) {
            const float MipLevel = ChetanJagsMipLevel(N, V, H, gRoughness);
    
            prefilteredColor += gi_EnvCubeMap.SampleLevel(gsamLinearClamp, L, MipLevel).rgb * NdotL;
            totalWeight += NdotL;
        }
    }
    prefilteredColor = prefilteredColor / totalWeight;
    
    return float4(prefilteredColor, 1.f);
}

#endif // __CONVOLUTESPECULARIRRADIANCE_HLSL__