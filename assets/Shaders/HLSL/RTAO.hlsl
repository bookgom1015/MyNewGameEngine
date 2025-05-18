#ifndef __RTAO_HLSL__
#define __RTAO_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../../assets/Shaders/HLSL/Samplers.hlsli"
#include "./../../../../assets/Shaders/HLSL/SSAO.hlsli"

typedef BuiltInTriangleIntersectionAttributes Attributes;

struct Ray {
    float3 Origin;
    float3 Direction;
};

struct RayPayload {
    float tHit;
};

ConstantBuffer<ConstantBuffers::AmbientOcclusionCB> cbAO : register(b0);

// Nonnumeric values cannot be added to a cbuffer.
RaytracingAccelerationStructure gBVH : register(t0);

Texture2D<ShadingConvention::GBuffer::PositionMapFormat>            gi_PositionMap : register(t1);
Texture2D<ShadingConvention::GBuffer::NormalMapFormat>              gi_NormalMap   : register(t2);
Texture2D<ShadingConvention::DepthStencilBuffer::DepthBufferFormat> gi_DepthMap    : register(t3);

RWTexture2D<float> go_AOCoefficientMap  : register(u0);
RWTexture2D<float> go_RayHitDistanceMap : register(u1);

bool TraceAORayAndReportIfHit(out float tHit, in Ray aoRay, in float TMax, in float3 surfaceNormal) {
    RayDesc ray;
	// Nudge the origin along the surface normal a bit to avoid starting from
	//  behind the surface due to float calculations imprecision.
    ray.Origin = aoRay.Origin + 0.1 * surfaceNormal;
    ray.Direction = aoRay.Direction;
    ray.TMin = 0;
    ray.TMax = TMax;

    RayPayload payload = { TMax };

    TraceRay(
		gBVH,   // AccelerationStructure
		0,      // RayFlags
		0xFF,   // AccelerationStructure
		0,      // RayContributionToHitGroupIndex
		0,      // MultiplierForGeometryContributionToHitGroupIndex
		0,      // MissShaderIndex
		ray,    // Ray
		payload // Ray
	);

    tHit = payload.tHit;

    return ShadingConvention::RTAO::HasAORayHitAnyGeometry(tHit);
}

float CalculateAO(out float tHit, in uint2 launchIndex, in Ray aoRay, in float3 surfaceNormal) {
    const float TMax = cbAO.OcclusionRadius;
    
    float occlusion = 0;    
    if (TraceAORayAndReportIfHit(tHit, aoRay, TMax, surfaceNormal)) {
        float3 hitPosition = aoRay.Origin + tHit * aoRay.Direction;
        float distZ = distance(aoRay.Origin, hitPosition);
        occlusion = SSAO::OcclusionFunction(distZ, cbAO.SurfaceEpsilon, cbAO.OcclusionFadeStart, cbAO.OcclusionFadeEnd);
    }
    
    return occlusion;
}

[shader("raygeneration")]
void RTAO_RayGen() {
    const uint2 LaunchIndex = DispatchRaysIndex().xy;
    const uint2 Dimensions = DispatchRaysDimensions().xy;
    
    float3 surfaceNormal = gi_NormalMap[LaunchIndex].xyz;
    float depth = gi_DepthMap[LaunchIndex];

    float tHit = ShadingConvention::RTAO::RayHitDistanceOnMiss;
    float aoCoefficient = ShadingConvention::RTAO::InvalidAOCoefficientValue;

    if (depth != ShadingConvention::RTAO::RayHitDistanceOnMiss) {
        float3 hitPosition = gi_PositionMap[LaunchIndex].xyz;

        const uint Seed = Random::InitRand(LaunchIndex.x + LaunchIndex.y * Dimensions.x, cbAO.FrameCount);

        float3 direction = Random::CosHemisphereSample(Seed, surfaceNormal);
        const float Flip = sign(dot(direction, surfaceNormal));
        direction = Flip * direction;

        float occlusionSum = 0;

        for (int i = 0; i < cbAO.SampleCount; ++i) {
            Ray aoRay = { hitPosition, direction };
            occlusionSum += CalculateAO(tHit, LaunchIndex, aoRay, surfaceNormal);
        }

        occlusionSum /= cbAO.SampleCount;
        aoCoefficient = 1 - occlusionSum;
    }

    go_AOCoefficientMap[LaunchIndex] = aoCoefficient;
    go_RayHitDistanceMap[LaunchIndex] = ShadingConvention::RTAO::HasAORayHitAnyGeometry(tHit) ? tHit : cbAO.OcclusionRadius;
}

[shader("closesthit")]
void RTAO_ClosestHit(inout RayPayload payload, in Attributes attrib) {
    payload.tHit = RayTCurrent();
}

[shader("miss")]
void RTAO_Miss(inout RayPayload payload) {
    payload.tHit = ShadingConvention::RTAO::RayHitDistanceOnMiss;
}

#endif // __RTAO_HLSL__