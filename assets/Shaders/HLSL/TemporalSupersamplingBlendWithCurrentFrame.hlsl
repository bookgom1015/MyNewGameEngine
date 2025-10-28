// [ Descriptions ]
//	2nd stage of temporal supersampling.
//	Blends current frame values with values reprojected from previous frame in stage 1.

#ifndef __TEMPORALSUPERSAMPLINGBLENDWIDTHCURRENTFRAME_HLSL__
#define __TEMPORALSUPERSAMPLINGBLENDWIDTHCURRENTFRAME_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"
#include "./../../../assets/Shaders/HLSL/SVGF.hlsli"
#include "./../../../assets/Shaders/HLSL/ValuePackaging.hlsli"

ConstantBuffer<ConstantBuffers::SVGF::BlendWithCurrentFrameCB> cbBlend : register(b0);

Texture2D<ShadingConvention::SVGF::ValueMapFormat>							gi_CurrentFrameValue						: register(t0);
Texture2D<ShadingConvention::SVGF::LocalMeanVarianceMapFormat>				gi_CurrentFrameLocalMeanVariance			: register(t1);
Texture2D<ShadingConvention::SVGF::RayHitDistanceMapFormat>					gi_CurrentFrameRayHitDistance				: register(t2);
Texture2D<ShadingConvention::SVGF::TSPPSquaredMeanRayHitDistanceMapFormat>	gi_ReprojTsppValueSquaredMeanRayHitDist		: register(t3);

RWTexture2D<ShadingConvention::SVGF::ValueMapFormat>					go_Value				: register(u0);
RWTexture2D<ShadingConvention::SVGF::ValueSquaredMeanMapFormat>			go_ValueSquaredMean		: register(u2);
RWTexture2D<ShadingConvention::SVGF::TSPPMapFormat>						go_Tspp					: register(u1);
RWTexture2D<ShadingConvention::SVGF::RayHitDistanceMapFormat>			go_RayHitDistance		: register(u3);
RWTexture2D<ShadingConvention::SVGF::VarianceMapFormat>					go_Variance				: register(u4);
RWTexture2D<ShadingConvention::SVGF::DisocclusionBlurStrengthMapFormat>	go_BlurStrength			: register(u5);

[numthreads(
    ShadingConvention::SVGF::ThreadGroup::Default::Width, 
    ShadingConvention::SVGF::ThreadGroup::Default::Height, 
    ShadingConvention::SVGF::ThreadGroup::Default::Depth)]
void CS(uint2 DTid : SV_DispatchThreadID) {
	const uint4 EncodedCachedValues = gi_ReprojTsppValueSquaredMeanRayHitDist[DTid];
	uint tspp = EncodedCachedValues.x;
	const float4 CachedValues = float4(tspp, f16tof32(EncodedCachedValues.yzw));

	bool isCurrentFrameValueActive = true;
	if (cbBlend.CheckerboardEnabled) {
		const bool IsEvenPixel = ((DTid.x + DTid.y) & 1) == 0;
		isCurrentFrameValueActive = cbBlend.CheckerboardEvenPixelActivated == IsEvenPixel;
	}

	float value = isCurrentFrameValueActive ? gi_CurrentFrameValue[DTid] : ShadingConvention::SVGF::InvalidValue;
	const bool IsValidValue = value != ShadingConvention::SVGF::InvalidValue;
	float valueSquaredMean = IsValidValue ? value * value : ShadingConvention::SVGF::InvalidValue;
	float rayHitDistance = ShadingConvention::SVGF::InvalidValue;
	float variance = ShadingConvention::SVGF::InvalidValue;

	if (tspp > 0) {
		const uint MaxTspp = 1 / cbBlend.MinSmoothingFactor;
		tspp = IsValidValue ? min(tspp + 1, MaxTspp) : tspp;

		float cachedValue = CachedValues.y;

		const float2 LocalMeanVariance = gi_CurrentFrameLocalMeanVariance[DTid];
		const float LocalMean = LocalMeanVariance.x;
		const float LocalVariance = LocalMeanVariance.y;
		if (cbBlend.ClampCachedValues) {
			const float LocalStdDev = max(cbBlend.StdDevGamma * sqrt(LocalVariance), cbBlend.ClampingMinStdDevTolerance);
			const float NonClampedCachedValue = cachedValue;

			// Clamp value to mean +/- std.dev of local neighborhood to supress ghosting on value changing due to other occluder movements.
			// Ref: Salvi2016, Temporal-Super-Sampling
			cachedValue = clamp(cachedValue, LocalMean - LocalStdDev, LocalMean + LocalStdDev);

			// Scale down the tspp based on how strongly the cached value got clamped to give more weight to new smaples.
			float tsppScale = saturate(cbBlend.ClampDifferenceToTsppScale * abs(cachedValue - NonClampedCachedValue));
			tspp = lerp(tspp, 0.f, tsppScale);
		}
		const float InvTspp = 1.f / tspp;
		const float MaxSmoothingFactor = 1.f;
		float a = cbBlend.ForceUseMinSmoothingFactor ? cbBlend.MinSmoothingFactor : max(InvTspp, cbBlend.MinSmoothingFactor);
		a = min(a, MaxSmoothingFactor);

		// TODO: use average weighting instead of exponential for the first few samples
		//  to even out the weights for the noisy start instead of giving first samples mush more weight than the rest.
		// Ref: Koskela2019, Blockwise Multi-Order Feature Regression for Real-Time Path-Tracing Reconstruction

		// Value.
		{
			value = IsValidValue ? lerp(cachedValue, value, a) : cachedValue;
		}

		// Value Squared Mean.
		{
			const float CachedSquaredMeanValue = CachedValues.z;
			valueSquaredMean = IsValidValue ? lerp(CachedSquaredMeanValue, valueSquaredMean, a) : CachedSquaredMeanValue;
		}

		// Variance.
		{
			float temporalVariance = valueSquaredMean - value * value;
			temporalVariance = max(0.f, temporalVariance); // Ensure variance doesn't go negative due to imprecision.
			variance = tspp >= cbBlend.MinTsppToUseTemporalVariance ? temporalVariance : LocalVariance;
			variance = max(0.1f, variance);
		}

		// RayHitDistance.
		{
			rayHitDistance = IsValidValue ? gi_CurrentFrameRayHitDistance[DTid] : 0.f;
			const float CachedRayHitDistance = CachedValues.y;
			rayHitDistance = IsValidValue ? lerp(CachedRayHitDistance, rayHitDistance, a) : CachedRayHitDistance;
		}
	}
	else if (IsValidValue) {
		tspp = 1;
		value = value;

		rayHitDistance = gi_CurrentFrameRayHitDistance[DTid];
		variance = gi_CurrentFrameLocalMeanVariance[DTid].y;
		valueSquaredMean = valueSquaredMean;
	}

	const float TsppRatio = min(tspp, cbBlend.BlurStrengthMaxTspp) / float(cbBlend.BlurStrengthMaxTspp);
	const float BlurStrength = pow(1.f - TsppRatio, cbBlend.BlurDecayStrength);

	go_Tspp[DTid] = tspp;
	go_Value[DTid] = value;
	go_ValueSquaredMean[DTid] = valueSquaredMean;
	go_RayHitDistance[DTid] = rayHitDistance;
	go_Variance[DTid] = variance;
	go_BlurStrength[DTid] = BlurStrength;
}

#endif // __TEMPORALSUPERSAMPLINGBLENDWIDTHCURRENTFRAME_HLSL__