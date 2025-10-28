//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

// Atrous Wavelet Transform Cross Bilateral Filter.
// Based on a 1st pass of [SVGF] filter.
// Ref: [Dammertz2010], Edge-Avoiding A-Trous Wavelet Transform for Fast Global Illumination Filtering
// Ref: [SVGF], Spatiotemporal Variance-Guided Filtering
// Ref: [RTGCH19] Ray Tracing Gems (Ch 19)

#ifndef __ATROUSWAVELETTRANSFORMFILTER_HLSLI__
#define __ATROUSWAVELETTRANSFORMFILTER_HLSLI__

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"
#include "./../../../assets/Shaders/HLSL/SVGF.hlsli"
#include "./../../../assets/Shaders/HLSL/ValuePackaging.hlsli"
#include "./../../../assets/Shaders/HLSL/Kernels.hlsli"
#include "./../../../assets/Shaders/HLSL/FloatPrecision.hlsli"

ConstantBuffer<ConstantBuffers::SVGF::AtrousWaveletTransformFilterCB> cbAtrous : register(b0);

SVGF_AtrousWaveletTransformFilter_RootConstants(b1)

Texture2D<ShadingConvention::SVGF::ValueMapFormat>	                gi_Value					: register(t0);
Texture2D<ShadingConvention::GBuffer::NormalDepthMapFormat>			gi_NormalDepth				: register(t1);
Texture2D<ShadingConvention::SVGF::VarianceMapFormat>				gi_Variance					: register(t2);
Texture2D<ShadingConvention::SVGF::RayHitDistanceMapFormat>			gi_HitDistance				: register(t3);
Texture2D<ShadingConvention::SVGF::DepthPartialDerivativeMapFormat>	gi_DepthPartialDerivative	: register(t4);

RWTexture2D<ShadingConvention::SVGF::ValueMapFormat>				go_FilteredValue			: register(u0);

float DepthThreshold(float depth, float2 ddxy, float2 pixelOffset) {
	float depthThreshold;
	if (cbAtrous.PerspectiveCorrectDepthInterpolation) {
		float2 newDdxy = SVGF::RemapDdxy(depth, ddxy, pixelOffset);
		depthThreshold = dot(1.f, abs(newDdxy));
	}
	else {
		depthThreshold = dot(1.f, abs(pixelOffset * ddxy));
	}
	return depthThreshold;
}

void AddFilterContribution(
		inout float weightedValueSum,
		inout float weightSum,
		in float value,
		in float stdDeviation,
		in float depth,
		in float3 normal,
		in float2 ddxy,
		in uint row,
		in uint col,
		in uint2 kernelStep,
		in uint2 DTid) {
	const float ValueSigma = cbAtrous.ValueSigma;
	const float NormalSigma = cbAtrous.NormalSigma;
	const float DepthSigma = cbAtrous.DepthSigma;

	int2 pixelOffset;
	float kernelWidth;
	float varianceScale = 1.f;

	pixelOffset = int2(row - FilterKernel::Radius, col - FilterKernel::Radius) * kernelStep;
	int2 id = int2(DTid) + pixelOffset;

	if (!ShaderUtil::IsWithinBounds(id, cbAtrous.TextureDim)) return;

	float iDepth;
	float3 iNormal;
	ValuePackaging::DecodeNormalDepth(gi_NormalDepth[id], iNormal, iDepth);

	float iValue = gi_Value[id];
	bool isValidValue = iValue != ShadingConvention::SVGF::InvalidValue;
	if (!isValidValue || iDepth == 0.f) return;

	// Calculate a weight for the neighbor's contribution.
	float w;
	{
		// Value based weight.
		// Lower value tolerance for the neighbors further apart. Prevents overbluring shapr value transition.
		const float ErrorOffset = 0.005f;
		float valueSigmaDistCoef = 1.f / length(pixelOffset);
		float variance = SVGF::ColorVariance(value, iValue);
		float e_x = -abs(variance) / (valueSigmaDistCoef * ValueSigma * stdDeviation + ErrorOffset);
		float w_x = exp(e_x);

		// Normal based weight.
		float w_n = pow(max(0.f, dot(normal, iNormal)), NormalSigma);

		// Depth based weight.
		float w_d;
		{
			float2 pixelOffsetForDepth = pixelOffset;

			// Account for sample offset in bilateral downsampled partial depth derivative buffer.
			if (cbAtrous.UsingBilateralDownsamplingBuffers) {
				float2 offsetSign = sign(pixelOffset);
				pixelOffsetForDepth = pixelOffset + offsetSign * float2(0.5, 0.5);
			}

			float depthFloatPrecision = FloatPrecision::FloatPrecision(max(depth, iDepth), cbAtrous.DepthNumMantissaBits);
			float depthThreshold = DepthThreshold(depth, ddxy, pixelOffsetForDepth);
			float depthTolerance = DepthSigma * depthThreshold + depthFloatPrecision;
			float delta = abs(depth - iDepth);
			// Avoid distinguishing initial values up to the float precision. Gets rid of banding due to low depth precision format.
			delta = max(0, delta - depthFloatPrecision);
			w_d = exp(-delta / depthTolerance);

			// Scale down contributions for samples beyond tolerance, but completely disable contribution for samples too far away.
			w_d *= w_d >= cbAtrous.DepthWeightCutoff;
		}

		// Filter kernel weight.
		float w_h = FilterKernel::Kernel[row][col];

		// Final weight.
		w = w_h * w_n * w_x * w_d;
	}

	weightedValueSum += w * iValue;
	weightSum += w;
}

[numthreads(
    ShadingConvention::SVGF::ThreadGroup::Atrous::Width, 
    ShadingConvention::SVGF::ThreadGroup::Atrous::Height, 
    ShadingConvention::SVGF::ThreadGroup::Atrous::Depth)]
void CS(uint2 DTid : SV_DispatchThreadID) {
	if (!ShaderUtil::IsWithinBounds(DTid, cbAtrous.TextureDim)) return;

	// Initialize values to the current pixel / center filter kernel value.
	float value = gi_Value[DTid];
	
	float3 normal;
	float depth;
	ValuePackaging::DecodeNormalDepth(gi_NormalDepth[DTid], normal, depth);

	const bool IsValidValue = value != ShadingConvention::SVGF::InvalidValue;
	float filteredValue = value;
	float variance = gi_Variance[DTid];

	if (depth != 0.f) {
		float2 ddxy = gi_DepthPartialDerivative[DTid];		
		float stdDeviation = 1.f;
		float weightSum = 0.f;
		float weightedValueSum = 0.f;

		if (IsValidValue) {
			float w = FilterKernel::Kernel[FilterKernel::Radius][FilterKernel::Radius];
			weightSum = w;
			weightedValueSum = weightSum * value;
			stdDeviation = sqrt(variance);
		}

		// Adaptive kernel size
		// Scale the kernel span based on AO ray hit distance.
		// This helps filter out lower frequency noise, a.k.a biling artifacts.
		// Ref: [RTGCH19]
		uint2 kernelStep = 0;
		if (cbAtrous.UseAdaptiveKernelSize && IsValidValue) {
			float avgRayHitDistance = gi_HitDistance[DTid];

			float perPixelViewAngle = cbAtrous.FovY / cbAtrous.TextureDim.y;
			float tan_a = tan(perPixelViewAngle);
			float2 projectedSurfaceDim = SVGF::ApproximateProjectedSurfaceDimensionsPerPixel(depth, ddxy, tan_a);

			// Calculate a kernel width as a ratio of hitDistance / projected surface dim per pixel.
			// Apply a non-linear factor based on relative rayHitDistance.
			// This is because average ray hit distance grows large fast if the closeby occluders cover only part if the hemisphere.
			// Having a smaller kernel for such cases helps preserve occlusion detail.
			float t = min(avgRayHitDistance / 22.0, 1); // 22 was seleted emprically.
			float k = gRayHitDistanceToKernelWidthScale * pow(t, gRayHitDistanceToKernelSizeScaleExponent);
			kernelStep = max(1, round(k * avgRayHitDistance / projectedSurfaceDim));

			uint2 targetKernelStep = clamp(kernelStep, (cbAtrous.MinKernelWidth - 1) / 2, (cbAtrous.MaxKernelWidth - 1) / 2);

			// TODO: additional options to explore
			// - non-uniform X, Y kernel radius cause visible streaking. Use same ratio across both X, Y? That may overblur one dimension at sharp angles.
			// - use larger kernel on lower tspp.
			// - use varying number of cycles for better spatial coverage over time, depending on the target kernel step. More cycles on larget kernels.
			uint2 adjustedKernelStep = lerp(1, targetKernelStep, cbAtrous.KernelRadiusLerfCoef);
			kernelStep = adjustedKernelStep;
		}

		if (variance >= cbAtrous.MinVarianceToDenoise) {
			// Add contributions from the neighborhood.
			[unroll]
			for (uint r = 0; r < FilterKernel::Width; ++r) {
				[unroll]
				for (uint c = 0; c < FilterKernel::Width; ++c) {
					if (r != FilterKernel::Radius || c != FilterKernel::Radius) {
						AddFilterContribution(
							weightedValueSum,
							weightSum,
							value,
							stdDeviation,
							depth,
							normal,
							ddxy,
							r, c,
							kernelStep,
							DTid
						);
					}
				}
			}
		}

		float smallValue = 1e-6f;
		if (weightSum > smallValue) filteredValue = weightedValueSum / weightSum;
		else filteredValue = ShadingConvention::SVGF::InvalidValue;
	}

	go_FilteredValue[DTid] = filteredValue;
}

#endif // __ATROUSWAVELETTRANSFORMFILTER_HLSLI__