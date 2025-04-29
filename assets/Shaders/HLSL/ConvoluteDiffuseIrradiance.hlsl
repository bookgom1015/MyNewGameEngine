#ifndef __CONVOLUTEDIFFUSEIRRADIANCE_HLSL__
#define __CONVOLUTEDIFFUSEIRRADIANCE_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef _CUBE_COORD
#define _CUBE_COORD
#endif 

#include "./../../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../../assets/Shaders/HLSL/Samplers.hlsli"

ConstantBuffer<ConstantBuffers::ProjectToCubeCB> cbProjectToCube : register(b0);

TextureCube<HDR_FORMAT> gi_CubeMap : register(t0);

struct VertexOut {
    float3 PosL : POSITION;
};

struct GeoOut {
    float4 PosH     : SV_Position;
    float3 PosL     : POSITION;
    uint ArrayIndex : SV_RenderTargetArrayIndex;
};

FitToCubeVertexShader

FitToCubeGeometryShader

float3 ConvoluteIrradiance(in float3 pos) {
	// The world vector acts as the normal of a tangent surface
	// from the origin, aligned to WorldPos. Given this normal, calculate all
	// incoming radiance of the environment. The result of this radiance
	// is the radiance of light coming from -Normal direction, which is what
	// we use in the PBR shader to sample irradiance.
    const float3 N = normalize(pos);

    float3 irradiance = (float3)0.f;

    float3 up = float3(0.f, 1.f, 0.f);
    const float3 Right = normalize(cross(up, N));
    up = normalize(cross(N, Right));

    uint numSamples = 0;
    const float SampleDelta = 0.05f;
    for (float phi = 0.f, phi_end = 2.f * PI; phi < phi_end; phi += SampleDelta) {
        for (float theta = 0.f, theta_end = 0.5f * PI; theta < theta_end; theta += SampleDelta) {
            const float CosTheta = cos(theta);
            const float SinTheta = sin(theta);

			// Spherical to cartesian (in tangent space)
            const float3 TangentSample = float3(SinTheta * cos(phi), SinTheta * sin(phi), CosTheta);
			// tangent space to world space
            const float3 SampleVec = TangentSample.x * Right + TangentSample.y * up + TangentSample.z * N;

            float4 gammaDecoded = gi_CubeMap.Sample(gsamLinearClamp, SampleVec);
            irradiance += gammaDecoded.rgb * CosTheta * SinTheta;
            ++numSamples;
        }
    }
    
    irradiance = PI * irradiance * (1.f / (float)numSamples);

    return irradiance;
}

HDR_FORMAT PS(in GeoOut pin) : SV_Target {
    const float3 Irradiance = ConvoluteIrradiance(pin.PosL);
    
    return float4(Irradiance, 1.f);
}

#endif // __CONVOLUTEDIFFUSEIRRADIANCE_HLSL__