// [ Descriptions ]
//  Stage 1 of Temporal-Supersampling.
//  Samples temporal cache via vectors/reserve reprojection.
//  If no valid values have been reterived from the cache, the tspp is set to 0.

#ifndef __TEMPORALSUPERSAMPLINGREVERSEREPROJECT_HLSL__
#define __TEMPORALSUPERSAMPLINGREVERSEREPROJECT_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../../assets/Shaders/HLSL/Samplers.hlsli"
#include "./../../../../assets/Shaders/HLSL/SVGF.hlsli"
#include "./../../../../assets/Shaders/HLSL/ValuePackaging.hlsli"
#include "./../../../../assets/Shaders/HLSL/CrossBilateralWeights.hlsli"

ConstantBuffer<ConstantBuffers::SVGF::CrossBilateralFilter> cbReproject : register(b0);

SVGF_TemporalSupersamplingReverseReproject_RootConstants(b1)

Texture2D<ShadingConvention::GBuffer::NormalDepthMapFormat>         gi_CurrentFrameNormalDepth : register(t0);
Texture2D<ShadingConvention::GBuffer::NormalDepthMapFormat>         gi_ReprojectedNormalDepth  : register(t1);
Texture2D<ShadingConvention::GBuffer::NormalDepthMapFormat>         gi_CachedNormalDepth       : register(t2);
Texture2D<ShadingConvention::SVGF::DepthPartialDerivativeMapFormat> gi_DepthPartialDerivative  : register(t3);
Texture2D<ShadingConvention::GBuffer::VelocityMapFormat>            gi_Velocity                : register(t4);
Texture2D<ShadingConvention::SVGF::ValueMapFormat_Color>            gi_CachedValue             : register(t5);
Texture2D<ShadingConvention::SVGF::ValueSquaredMeanMapFormat_Color> gi_CachedValueSquaredMean  : register(t6);
Texture2D<ShadingConvention::SVGF::TSPPMapFormat>                   gi_CachedTspp              : register(t7);
Texture2D<ShadingConvention::SVGF::RayHitDistanceFormat>            gi_CachedRayHitDistance    : register(t8);

RWTexture2D<ShadingConvention::SVGF::TSPPMapFormat>                          go_CachedTspp              : register(u0);
RWTexture2D<ShadingConvention::SVGF::ValueMapFormat_Color>                   go_CachedValue             : register(u1);
RWTexture2D<ShadingConvention::SVGF::ValueSquaredMeanMapFormat_Color>        go_CachedSquaredMean       : register(u2);
RWTexture2D<ShadingConvention::SVGF::TSPPSquaredMeanRayHitDistanceMapFormat> go_ReprojectedCachedValues : register(u3);

float4 BilateralResampleWeights(
		in float targetDepth,
		in float3 targetNormal,
		in float4 sampleDepths,
		in float3 sampleNormals[4],
		in float2 targetOffset,
		in uint2 targetIndex,
		in int2 cacheIndices[4],
		in float2 ddxy) {
    const bool4 IsWithinBounds = bool4(
		SVGF::IsWithinBounds(cacheIndices[0], gTexDim),
		SVGF::IsWithinBounds(cacheIndices[1], gTexDim),
		SVGF::IsWithinBounds(cacheIndices[2], gTexDim),
		SVGF::IsWithinBounds(cacheIndices[3], gTexDim)
	);

    CrossBilateral::BilinearDepthNormal::Parameters params;
    params.Depth.Sigma = cbReproject.DepthSigma;
    params.Depth.WeightCutoff = 0.5f;
    params.Depth.NumMantissaBits = cbReproject.DepthNumMantissaBits;
    params.Normal.Sigma = 1.1f; // Bump the sigma a bit to add tolerance for slight geometry misalignments and/or format precision limitations.
    params.Normal.SigmaExponent = 32;

    const float4 BilinearDepthNormalWeights = CrossBilateral::BilinearDepthNormal::GetWeights(
		targetDepth,
		targetNormal,
		targetOffset,
		ddxy,
		sampleDepths,
		sampleNormals,
		params);

    const float4 Weights = IsWithinBounds * BilinearDepthNormalWeights;

    return Weights;
}

[numthreads(
    ShadingConvention::SVGF::ThreadGroup::Default::Width, 
    ShadingConvention::SVGF::ThreadGroup::Default::Height, 
    ShadingConvention::SVGF::ThreadGroup::Default::Depth)]
void CS(in uint2 DTid : SV_DispatchThreadID) {
    if (!SVGF::IsWithinBounds(DTid, gTexDim)) return;
    
    const uint ReprojNormalDepth = gi_ReprojectedNormalDepth[DTid];    
    
    const float2 TexC = (DTid + 0.5) * gInvTexDim;    
    const float2 Velocity = gi_Velocity.SampleLevel(gsamLinearClamp, TexC, 0);
    
    if (ShadingConvention::GBuffer::IsValidNormalDepth(ReprojNormalDepth) || Velocity.x > 100.f) {
        go_CachedTspp[DTid] = 0;
        return;
    }
    
    float3 reprojNormal;
    float reprojDepth;
    ValuePackaging::DecodeNormalDepth(ReprojNormalDepth, reprojNormal, reprojDepth);

    const float2 CacheTexC = TexC - Velocity;

	// Find the nearest integer index samller than the texture position.
	// The floor() ensures the that value sign is taken into consideration.
    int2 topLeftCacheIndex = floor(CacheTexC * gTexDim - 0.5);
    float2 adjustedCacheTex = (topLeftCacheIndex + 0.5) * gInvTexDim;

    float2 cachePixelOffset = CacheTexC * gTexDim - 0.5 - topLeftCacheIndex;

    const int2 srcIndexOffsets[4] = { { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 } };

    int2 cacheIndices[4] = {
        topLeftCacheIndex + srcIndexOffsets[0],
		topLeftCacheIndex + srcIndexOffsets[1],
		topLeftCacheIndex + srcIndexOffsets[2],
		topLeftCacheIndex + srcIndexOffsets[3]
    };

    float3 cacheNormals[4];
    float4 cacheDepths = 0;
	{
        uint4 packed = gi_CachedNormalDepth.GatherRed(gsamPointClamp, adjustedCacheTex).wzxy;
        
		[unroll]
        for (int i = 0; i < 4; ++i) {
            ValuePackaging::DecodeNormalDepth(packed[i], cacheNormals[i], cacheDepths[i]);
        }
    }

    float2 ddxy = gi_DepthPartialDerivative.SampleLevel(gsamPointClamp, TexC, 0);

    float4 weights = BilateralResampleWeights(reprojDepth, reprojNormal, cacheDepths, cacheNormals, cachePixelOffset, DTid, cacheIndices, ddxy);

    uint2 size;
    gi_CachedValue.GetDimensions(size.x, size.y);

    float dx = 1.0 / size.x;
    float dy = 1.0 / size.y;

	// Invalidate weights for invalid values in the cache.
    float4 vCacheValues[4];
    vCacheValues[0] = gi_CachedValue.SampleLevel(gsamPointClamp, adjustedCacheTex, 0);
    vCacheValues[1] = gi_CachedValue.SampleLevel(gsamPointClamp, adjustedCacheTex + float2(dx, 0), 0);
    vCacheValues[2] = gi_CachedValue.SampleLevel(gsamPointClamp, adjustedCacheTex + float2(0, dy), 0);
    vCacheValues[3] = gi_CachedValue.SampleLevel(gsamPointClamp, adjustedCacheTex + float2(dx, dy), 0);

    if (vCacheValues[0].a > 0)
        weights.x = 0;
    if (vCacheValues[1].a > 0)
        weights.y = 0;
    if (vCacheValues[2].a > 0)
        weights.z = 0;
    if (vCacheValues[3].a > 0)
        weights.w = 0;

    float weightSum = dot(1, weights);

    float4 cachedValue = 0;
    float4 cachedValueSquaredMean = 0;
    float cachedRayHitDist = 0;

    uint tspp;
    if (weightSum > 0.001) {
        uint4 vCachedTspp = gi_CachedTspp.GatherRed(gsamPointClamp, adjustedCacheTex).wzxy;
		// Enforce tspp of at least 1 for reprojection for valid values.
		// This is because the denoiser will fill in invalid values with filtered 
		// ones if it can. But it doesn't increase tspp.
        vCachedTspp = max(1, vCachedTspp);

        float4 nWeights = weights / weightSum; // Normalize the weights.

		// Scale the tspp by the total weight. This is to keep the tspp low for 
		// total contributions that have very low reprojection weight. While its preferred to get 
		// a weighted value even for reprojections that have low weights but still
		// satisfy consistency tests, the tspp needs to be kept small so that the Target calculated values
		// are quickly filled in over a few frames. Otherwise, bad estimates from reprojections,
		// such as on disocclussions of surfaces on rotation, are kept around long enough to create 
		// visible streaks that fade away very slow.
		// Example: rotating camera around dragon's nose up close. 
        const float TsppScale = 1;

        float cachedTspp = TsppScale * dot(nWeights, vCachedTspp);
        tspp = round(cachedTspp);

        if (tspp > 0) {
			{
				[unroll]
                for (int i = 0; i < 4; ++i)
                    cachedValue += nWeights[i] * vCacheValues[i];
            }

            float4 vCachedValueSquaredMeans[4];
            vCachedValueSquaredMeans[0] = gi_CachedValueSquaredMean.SampleLevel(gsamPointClamp, adjustedCacheTex, 0);
            vCachedValueSquaredMeans[1] = gi_CachedValueSquaredMean.SampleLevel(gsamPointClamp, adjustedCacheTex + float2(dx, 0), 0);
            vCachedValueSquaredMeans[2] = gi_CachedValueSquaredMean.SampleLevel(gsamPointClamp, adjustedCacheTex + float2(0, dy), 0);
            vCachedValueSquaredMeans[3] = gi_CachedValueSquaredMean.SampleLevel(gsamPointClamp, adjustedCacheTex + float2(dx, dy), 0);
			{
				[unroll]
                for (int i = 0; i < 4; ++i)
                    cachedValueSquaredMean += nWeights[i] * vCachedValueSquaredMeans[i];
            }

            float4 vCachedRayHitDist = gi_CachedRayHitDistance.GatherRed(gsamPointClamp, adjustedCacheTex).wzxy;
            cachedRayHitDist = dot(nWeights, vCachedRayHitDist);
        }
    }
    else {
		// No valid values can be retrieved from the cache.
		// TODO: try a greater cache footprint to find useful samples,
		//   For example a 3x3 pixel cache footprint or use lower mip cache input.
        tspp = 0;
    }

    go_CachedTspp[DTid] = tspp;
    go_CachedValue[DTid] = cachedValue;
    go_CachedSquaredMean[DTid] = cachedValueSquaredMean;
    go_ReprojectedCachedValues[DTid] = uint4(tspp, f32tof16(float3(cachedRayHitDist, 0, 0)));
}

#endif // __TEMPORALSUPERSAMPLINGREVERSEREPROJECT_HLSL__
