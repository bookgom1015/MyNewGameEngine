#ifndef __RTAO_HLSL__
#define __RTAO_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"
#include "./../../../assets/Shaders/HLSL/ValuePackaging.hlsli"
#include "./../../../assets/Shaders/HLSL/RaySorting.hlsli"
#include "./../../../assets/Shaders/HLSL/SSAO.hlsli"

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

Texture2D<ShadingConvention::GBuffer::PositionMapFormat>            gi_PositionMap                          : register(t1);
Texture2D<ShadingConvention::GBuffer::NormalDepthMapFormat>         gi_NormalDepthMap                       : register(t2);
Texture2D<ShadingConvention::GBuffer::NormalDepthMapFormat>         gi_RayDirectionOriginDepthMap           : register(t3);
Texture2D<uint2>                                                    gi_TexAOSortedToSourceRayIndexOffsetMap : register(t4);

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
    
    float3 surfaceNormal;
    float depth;
    ValuePackaging::DecodeNormalDepth(gi_NormalDepthMap[LaunchIndex], surfaceNormal, depth);

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

// Retrieves 2D source and sorted ray indices from a 1D ray index where
// - every valid (i.e. is within ray tracing buffer dimensions) 1D index maps to a valid 2D index.
// - pixels are row major within a ray group.
// - ray groups are row major within the raytracing buffer dimensions.
// - rays are sorted per ray group.
// Overflowing ray group dimensions on the borders are clipped to valid raytracing dimnesions.
// Returns whether the retrieved ray is active.
bool Get2DRayIndices(out uint2 sortedRayIndex2D, out uint2 srcRayIndex2D, in uint index1D) {
    const uint2 RayGroupDim = uint2(ShadingConvention::RaySorting::RayGroup::Width, ShadingConvention::RaySorting::RayGroup::Height);

    // Find the ray group row index.
    const uint NumValidPixelsInRow = cbAO.TextureDim.x;
    const uint RowOfRayGroupSize = RayGroupDim.y * NumValidPixelsInRow;
    const uint RayGroupRowIndex = index1D / RowOfRayGroupSize;

    // Find the ray group column index.
    const uint NumValidPixelsInColumn = cbAO.TextureDim.y;
    const uint NumRowsInCurrentRayGroup = min((RayGroupRowIndex + 1) * RayGroupDim.y, NumValidPixelsInColumn) - RayGroupRowIndex * RayGroupDim.y;
    const uint CurrentRow_RayGroupSize = NumRowsInCurrentRayGroup * RayGroupDim.x;
    const uint Index1DWithinRayGroupRow = index1D - RayGroupRowIndex * RowOfRayGroupSize;
    const uint RayGroupColumnIndex = Index1DWithinRayGroupRow / CurrentRow_RayGroupSize;
    const uint2 RayGroupIndex = uint2(RayGroupColumnIndex, RayGroupRowIndex);

    // Find the thread offset index within the ray group.
    const uint CurrentRayGroup_index1D = Index1DWithinRayGroupRow - RayGroupIndex.x * CurrentRow_RayGroupSize;
    const uint CurrentRayGroupWidth = min((RayGroupIndex.x + 1) * RayGroupDim.x, NumValidPixelsInRow) - RayGroupIndex.x * RayGroupDim.x;
    const uint RayThreadRowIndex = CurrentRayGroup_index1D / CurrentRayGroupWidth;
    const uint RayThreadColumnIndex = CurrentRayGroup_index1D - RayThreadRowIndex * CurrentRayGroupWidth;
    const uint2 RayThreadIndex = uint2(RayThreadColumnIndex, RayThreadRowIndex);

    // Get the corresponding source index
    sortedRayIndex2D = RayGroupIndex * RayGroupDim + RayThreadIndex;
    const uint2 RayGroupBase = RayGroupIndex * RayGroupDim;
    const uint2 RayGroupRayIndexOffset = gi_TexAOSortedToSourceRayIndexOffsetMap[sortedRayIndex2D];
    srcRayIndex2D = RayGroupBase + GetRawRayIndexOffset(RayGroupRayIndexOffset);

    return IsActiveRay(RayGroupRayIndexOffset);
}

[shader("raygeneration")]
void RTAO_RayGen_RaySorted() {
    const uint DTid_1D = DispatchRaysIndex().x;
    
    uint2 srcRayIndex;
    uint2 sortedRayIndex;
    const bool bIsActiveRay = Get2DRayIndices(sortedRayIndex, srcRayIndex, DTid_1D);

    uint2 srcRayIndexFullRes = srcRayIndex;
    if (cbAO.CheckerboardRayGenEnabled) {
        const uint PixelStepX = 2;
        const bool IsEvenPixelY = (srcRayIndex.y & 1) == 0;
        const uint PixelOffsetX = IsEvenPixelY != cbAO.EvenPixelsActivated;
        srcRayIndexFullRes.x = srcRayIndex.x * PixelStepX + PixelOffsetX;
    }

    float tHit = ShadingConvention::RTAO::RayHitDistanceOnMiss;
    float aoCoefficient = ShadingConvention::RTAO::InvalidAOCoefficientValue;
    if (bIsActiveRay) {
        float dummy;
        float3 rayDirection;
        ValuePackaging::DecodeNormalDepth(gi_RayDirectionOriginDepthMap[srcRayIndex], rayDirection, dummy);
        
        float3 surfaceNormal;
        float depth;
        ValuePackaging::DecodeNormalDepth(gi_NormalDepthMap[srcRayIndexFullRes], surfaceNormal, depth);

        const float3 HitPosition = gi_PositionMap[srcRayIndexFullRes].xyz;
        
        Ray AORay = { HitPosition, rayDirection };
        aoCoefficient = CalculateAO(tHit, srcRayIndexFullRes, AORay, surfaceNormal);
    }

    const uint2 OutPixel = srcRayIndexFullRes;

    go_AOCoefficientMap[OutPixel] = aoCoefficient;
    go_RayHitDistanceMap[OutPixel] = ShadingConvention::RTAO::HasAORayHitAnyGeometry(tHit) ? tHit : cbAO.OcclusionRadius;
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