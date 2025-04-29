#ifndef __INTEGRATEBRDF_HLSL__
#define __INTEGRATEBRDF_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef _FIT_TO_SCREEN_COORD
#define _FIT_TO_SCREEN_COORD
#endif

#include "./../../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../../assets/Shaders/HLSL/Samplers.hlsli"
#include "./../../../../assets/Shaders/HLSL/BRDF.hlsli"

ConstantBuffer<ConstantBuffers::PassCB> cbPass : register(b0);

struct VertexOut {
    float4 PosH : SV_Position;
    float2 TexC : TexCoord;
};

static const uint SAMPLE_COUNT = 1024;

FitToScreenVertexShader

FitToScreenMeshShader

float2 IntegrateBRDF(in float NdotV, in float roughness) {
    roughness = max(roughness, 0.04f);

    float3 V;
    V.x = sqrt(1.f - NdotV * NdotV);
    V.y = 0.f;
    V.z = NdotV;

    float A = 0.f;
    float B = 0.f;

    float3 N = float3(0.f, 0.f, 1.f);

	[loop]
    for (uint i = 0; i < SAMPLE_COUNT; ++i) {
        const float2 Xi = Hammersley(i, SAMPLE_COUNT);
        const float3 H = ImportanceSampleGGX(Xi, N, roughness);
        const float3 L = normalize(2.f * dot(V, H) * H - V);

        const float NdotL = max(L.z, 0.f);
        const float NdotH = max(H.z, 0.f);
        const float VdotH = max(dot(V, H), 0.f);

        if (NdotL > 0.f) {
            const float G = GeometrySmith_IBL(N, V, L, roughness);
            const float G_Vis = (G * VdotH) / (NdotH * NdotV);
            const float Fc = pow(1.f - VdotH, 5.f);

            A += (1.f - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }

    const float Inv = 1.f / (float) SAMPLE_COUNT;

    A *= Inv;
    B *= Inv;

    return float2(A, B);
}

float2 PS(in VertexOut pin) : SV_Target {
    return IntegrateBRDF(pin.TexC.x, pin.TexC.y);
}

#endif // __INTEGRATEBRDF_HLSL__