#ifndef __DISOCCLUSIONBLUR3X3_HLSL__
#define __DISOCCLUSIONBLUR3X3_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"
#include "./../../../assets/Shaders/HLSL/SVGF.hlsli"
#include "./../../../assets/Shaders/HLSL/ValuePackaging.hlsli"

#define GAUSSIAN_KERNEL_3X3
#include "./../../../assets/Shaders/HLSL/Kernels.hlsli"

SVGF_DisocclusionBlur_RootConstants(b0)

Texture2D<ShadingConvention::DepthStencilBuffer::DepthBufferFormat>		gi_Depth				: register(t0);
Texture2D<ShadingConvention::SVGF::DisocclusionBlurStrengthMapFormat>	gi_BlurStrength			: register(t1);
Texture2D<ShadingConvention::GBuffer::RoughnessMetalnessMapFormat>		gi_RoughnessMetalness	: register(t2);

RWTexture2D<ShadingConvention::SVGF::ValueMapFormat>					gio_Value				: register(u0);

// Group shared memory cache for the row aggregated results.
static const uint NumValuesToLoadPerRowOrColumn = ShadingConvention::SVGF::ThreadGroup::Default::Width + (FilterKernel::Width - 1);

groupshared uint PackedValueDepthCache[NumValuesToLoadPerRowOrColumn][8];
// 32bit float filtered value.
groupshared float FilteredResultCache[NumValuesToLoadPerRowOrColumn][8];

// Find a dispatchThreadID with steps in between the group threads and groups interleaved to cover all pixels.
uint2 GetPixelIndex(in uint2 Gid, in uint2 GTid) {
	const uint2 GroupDim = uint2(8, 8);
	const uint2 GroupBase = (Gid / gStep) * GroupDim * gStep + Gid % gStep;
	const uint2 GroupThreadOffset = GTid * gStep;
	const uint2 sDTid = GroupBase + GroupThreadOffset;
	return sDTid;
}

// Load up to 16x16 pixels and filter them horizontally.
// The output is cached in Shared Memory and constants NumRows x 8 results.
void FilterHorizontally(in uint2 Gid, in uint GI) {
	const uint2 GroupDim = uint2(8, 8);

	// Processes the thread group as row-major 4x16, where each sub group of 16 threads processes one row.
	// Each thread loads up to 4 values, with the sub groups loading rows interleaved.
	// Loads up to 4x16x4 == 256 input values.
	const uint2 GTid4x16_row0 = uint2(GI % 16, GI / 16);
	const int2 GroupKernelBasePixel = GetPixelIndex(Gid, 0) - int(FilterKernel::Radius * gStep);
	const uint NumRowsToLoadPerThread = 4;
	const uint RowBaseWaveLaneIndex = (WaveGetLaneIndex() / 16) * 16;

	[unroll]
	for (uint i = 0; i < NumRowsToLoadPerThread; ++i) {
		uint2 GTid4x16 = GTid4x16_row0 + uint2(0, i * 4);
		if (GTid4x16.y >= NumValuesToLoadPerRowOrColumn) break;

		// Load all the contributing columns for each row.
		int2 pixel = GroupKernelBasePixel + GTid4x16 * gStep;
		float value = ShadingConvention::SVGF::InvalidValue;
		float depth = 0.f;

		// The lane is out of bounds of the GroupDim + kernel, but could be within bounds of the input texture,
		//  so don't read it from the texture.
		// However, we need to keep it as an active lane for a below split sum.
		if (GTid4x16.x < NumValuesToLoadPerRowOrColumn && ShaderUtil::IsWithinBounds(pixel, gTextureDim)) {
			value = gio_Value[pixel];
			depth = gi_Depth[pixel];
		}

		// Cache the kernel center values.
		if (ShaderUtil::IsInRange(GTid4x16.x, FilterKernel::Radius, FilterKernel::Radius + GroupDim.x - 1)) {
			const int Row = GTid4x16.y;
			const int Col = GTid4x16.x - FilterKernel::Radius;
			
			PackedValueDepthCache[Row][Col] = ValuePackaging::Float2ToHalf(float2(value, depth));
		}

		// Filter the values for the first GroupDim columns.
		{
			// Accumulate for the whole kernel width.
			float weightedValueSum = 0.f;
			float gaussianWeightedValueSum = 0.f;
			float weightSum = 0.f;
			float gaussianWeightedSum = 0.f;

			// Since a row uses 16 lanes, but we only need to calculate the aggregate for the first half (8) lanes,
			//  split the kernel wide aggregation among the first 8 and the second 8 lanes, and then combine them.

			// Get the lane index that has the first value for a kernel in this lane.
			const uint RowKernelStartLaneIndex =
				(RowBaseWaveLaneIndex + GTid4x16.x)
				- (GTid4x16.x < GroupDim.x ? 0 : GroupDim.x);

			// Get values for the kernel center.
			const uint kcLaneIndex = RowKernelStartLaneIndex + FilterKernel::Radius;
			const float kcValue = WaveReadLaneAt(value, kcLaneIndex);
			const float kcDepth = WaveReadLaneAt(depth, kcLaneIndex);

			// Initialize the first 8 lanes to the center cell contribution of the kernel.
			// This covers the remainder of 1 in FilterKernel::Width / 2 used in the loop below.
			{
				if (GTid4x16.x < GroupDim.x && kcValue != ShadingConvention::SVGF::InvalidValue && 
						kcDepth != ShadingConvention::DepthStencilBuffer::InvalidDepthValue) {
					const float w_h = FilterKernel::Kernel1D[FilterKernel::Radius];
					gaussianWeightedValueSum = w_h * kcValue;
					gaussianWeightedSum = w_h;
					weightedValueSum = gaussianWeightedValueSum;
					weightSum = w_h;
				}
			}

			// Second 8 lanes start just past the kernel center.
			const uint KernelCellIndexOffset = GTid4x16.x < GroupDim.x ?
				0 : (FilterKernel::Radius + 1); // Skip over the already accumulated center cell of the kernel.

			// For all columns in the kernel.
			for (uint c = 0; c < FilterKernel::Radius; ++c) {
				const uint KernelCellIndex = KernelCellIndexOffset + c;

				const uint LaneToReadFrom = RowKernelStartLaneIndex + KernelCellIndex;
				const float cValue = WaveReadLaneAt(value, LaneToReadFrom);
				const float cDepth = WaveReadLaneAt(depth, LaneToReadFrom);

				if (cValue != ShadingConvention::SVGF::InvalidValue && 
						kcDepth != ShadingConvention::DepthStencilBuffer::InvalidDepthValue && 
						cDepth != ShadingConvention::DepthStencilBuffer::InvalidDepthValue) {
					float w_h = FilterKernel::Kernel1D[KernelCellIndex];

					// Simple depth test with tolerance growing as the kernel radius increases.
					// Goal is to prevent values too far apart to blend together, while having
					//  the test being relaxed enough to get a strong blurring result.
					const float DepthThreshold = 0.05f + gStep * 0.001f * abs(int(FilterKernel::Radius) - c);
					const float w_d = abs(kcDepth - cDepth) <= DepthThreshold * kcDepth;
					const float w = w_h * w_d;

					weightedValueSum += w * cValue;
					weightSum += w;
					gaussianWeightedValueSum += w_h * cValue;
					gaussianWeightedSum += w_h;
				}
			}

			// Combine the sub-results.
			const uint LaneToReadFrom = min(WaveGetLaneCount() - 1, RowBaseWaveLaneIndex + GTid4x16.x + GroupDim.x);
			weightedValueSum += WaveReadLaneAt(weightedValueSum, LaneToReadFrom);
			weightSum += WaveReadLaneAt(weightSum, LaneToReadFrom);
			gaussianWeightedValueSum += WaveReadLaneAt(gaussianWeightedValueSum, LaneToReadFrom);
			gaussianWeightedSum += WaveReadLaneAt(gaussianWeightedSum, LaneToReadFrom);

			// Store only the valid results, i.e. first GroupDim columns.
			if (GTid4x16.x < GroupDim.x) {
				const float GaussianFilteredValue = gaussianWeightedSum > 1e-6 ? 
					gaussianWeightedValueSum / gaussianWeightedSum : ShadingConvention::SVGF::InvalidValue;
				const float FilteredValue = weightSum > 1e-6 ? weightedValueSum / weightSum : GaussianFilteredValue;

				FilteredResultCache[GTid4x16.y][GTid4x16.x] = FilteredValue;
			}
		}
	}
}

void FilterVertically(in uint2 DTid, in uint2 GTid, in float blurStrength) {
	// Kernel center values.
	const float2 kcValueDepth = ValuePackaging::HalfToFloat2(PackedValueDepthCache[GTid.y + FilterKernel::Radius][GTid.x]);
    const float kcValue = kcValueDepth.x;
    const float kcDepth = kcValueDepth.y;
	
	float filteredValue = kcValue;

	if (blurStrength >= 0.01f && kcDepth != ShadingConvention::DepthStencilBuffer::InvalidDepthValue) {
		float weightedValueSum = 0.f;
		float gaussianWeightedValueSum = 0.f;
		float weightSum = 0.f;
		float gaussianWeightSum = 0.f;

		// For all rows in the kernel.
		[unroll]
		for (uint r = 0; r < FilterKernel::Width; ++r) {
			uint rowID = GTid.y + r;

			const float2 rUnpackedValueDepth = ValuePackaging::HalfToFloat2(PackedValueDepthCache[rowID][GTid.x]);
            const float rDepth = rUnpackedValueDepth.y;			
			const float rFilteredValue = FilteredResultCache[rowID][GTid.x];

			if (rDepth != ShadingConvention::DepthStencilBuffer::InvalidDepthValue && 
					rFilteredValue != ShadingConvention::SVGF::InvalidValue) {
				const float w_h = FilterKernel::Kernel1D[r];

				// Simple depth test with tolerance growing as the kernel radius increases.
				// Goal is to prevent values too far apart to blend together, while having 
				// the test being relaxed enough to get a strong blurring result.
				const float DepthThreshold = 0.05f + gStep * 0.001f * abs(int(FilterKernel::Radius) - int(r));
				const float w_d = abs(kcDepth - rDepth) <= DepthThreshold * kcDepth;
				const float w = w_h * w_d;

				weightedValueSum += w * rFilteredValue;
				weightSum += w;
				gaussianWeightedValueSum += w_h * rFilteredValue;
				gaussianWeightSum += w_h;
			}
		}

		float gaussianFilteredValue = gaussianWeightSum > 1e-6 ? 
			gaussianWeightedValueSum / gaussianWeightSum : ShadingConvention::SVGF::InvalidValue;
        filteredValue = weightSum > 1e-6 ? weightedValueSum / weightSum : gaussianFilteredValue;
        filteredValue = filteredValue != ShadingConvention::SVGF::InvalidValue ? 
			lerp(kcValue, filteredValue, blurStrength) : filteredValue;
	}

	gio_Value[DTid] = filteredValue;
}

[numthreads(
    ShadingConvention::SVGF::ThreadGroup::Default::Width, 
    ShadingConvention::SVGF::ThreadGroup::Default::Height, 
    ShadingConvention::SVGF::ThreadGroup::Default::Depth)]
void CS(in uint2 Gid : SV_GroupID, in uint2 GTid : SV_GroupThreadID, 
		in uint GI : SV_GroupIndex, in uint2 DTid : SV_DispatchThreadID) {
	const uint2 sDTid = GetPixelIndex(Gid, GTid);
	// Pass through if all pixels have 0 blur strength set.
	float blurStrength;
	{
		if (GI == 0) FilteredResultCache[0][0] = ShadingConvention::SVGF::InvalidValue;
		GroupMemoryBarrierWithGroupSync();

		blurStrength = gi_BlurStrength[sDTid];

		const float MinBlurStrength = 0.01f;
		const bool ValueNeedsFiltering = blurStrength >= MinBlurStrength;
		if (ValueNeedsFiltering) FilteredResultCache[0][0] = 1.f;

		GroupMemoryBarrierWithGroupSync();

		if (all(FilteredResultCache[0][0] == ShadingConvention::SVGF::InvalidValue)) return;
	}

	FilterHorizontally(Gid, GI);
	GroupMemoryBarrierWithGroupSync();

	FilterVertically(sDTid, GTid, blurStrength);
}

#endif // __DISOCCLUSIONBLUR3X3_HLSL__