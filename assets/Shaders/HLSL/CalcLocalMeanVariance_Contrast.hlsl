#ifndef __CALCLOCALMEANVARIANCE_HLSL__
#define __CALCLOCALMEANVARIANCE_HLSL__

// ---- Descriptions -------------------------------------------------------------------
// Calculate local mean and variance via a separable kernel and using wave intrinsics.
// Requirments:
//  - Wave lane size 16 or higher
//  - WaveReadLaneAt() with any to any to wave read lane support
// Supports:
//  - Up to 9x9 kernels
//  - Checkerboard on/off input. If enabled, outputs only for active pixels.
//     Active pixel is a pixel on the checkerboard pattern and has a valid /
//     generated value for it. The kernel is stretched in y direction
//     to sample only from active pixels.
// -------------------------------------------------------------------------------------

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"
#include "./../../../assets/Shaders/HLSL/ValuePackaging.hlsli"
#include "./../../../assets/Shaders/HLSL/SVGF.hlsli"

ConstantBuffer<ConstantBuffers::SVGF::CalcLocalMeanVarianceCB> cbLocalMeanVar : register(b0);

Texture2D<ShadingConvention::SVGF::ValueMapFormat_Contrast>      gi_Value             : register(t0);
RWTexture2D<ShadingConvention::SVGF::LocalMeanVarianceMapFormat> go_LocalMeanVariance : register(u0);

#include "./../../../assets/Shaders/HLSL/CalcLocalMeanVariance.hlsli"

// Group shared memory cache for the row aggregated results.
groupshared uint2 PackedRowResultCache[16][8]; // 16bit float valueSum, squared valueSum
groupshared uint NumValuesCache[16][8];

// Load up to 16x16 pixels and filter them horizontally.
// The output is cached in shared memory and contains NumRows x 8 results.
void FilterHorizontally(uint2 Gid, uint GI) {
    const uint2 GroupDim = uint2(8, 8);
    const uint NumValuesToLoadPerRowOrColumn = GroupDim.x + (cbLocalMeanVar.KernelWidth - 1);

	// Processes the thread group as row-major 4x16, where each sub group of 16 threads processes one row.
	// Each thread loads up to 4 values, with the subgroups loading rows interleaved.
	// Loads up to 4x16x4 == 256 input values.
    const uint2 GTid4x16_row0 = uint2(GI % 16, GI / 16);
    const int2 KernelBasePixel = (Gid * GroupDim - int(cbLocalMeanVar.KernelRadius)) * int2(1, cbLocalMeanVar.PixelStepY);
    const uint NumRowsToLoadPerThread = 4;
    const uint RowBaseWaveLaneIndex = (WaveGetLaneIndex() / 16) * 16;

	[unroll]
    for (uint i = 0; i < NumRowsToLoadPerThread; ++i) {
        const uint2 GTid4x16 = GTid4x16_row0 + uint2(0, i * 4);
        if (GTid4x16.y >= NumValuesToLoadPerRowOrColumn) {
            if (GTid4x16.x < GroupDim.x)
                NumValuesCache[GTid4x16.y][GTid4x16.x] = 0;
            break;
        }

		// Load all the contributing columns for each row.
        const int2 Pixel = CalcLocalMeanVariance::GetActivePixelIndex(KernelBasePixel + GTid4x16 * int2(1, cbLocalMeanVar.PixelStepY));
        float value = ShadingConvention::SVGF::InvalidContrastValue;

		// The lane is out of bounds of the GroupDim * kernel, but could be within bounds of the input texture, so don't read it form the texture.
		// However, we need to keep it as an active lane for a below split sum.
        if (GTid4x16.x < NumValuesToLoadPerRowOrColumn && SVGF::IsWithinBounds(Pixel, cbLocalMeanVar.TextureDim))
            value = gi_Value[Pixel];

		// Filter the values for the first GroupDim columns.
		{
			// Accumulate for the whole kernel width.
            float valueSum = 0;
            float squaredValueSum = 0;
            uint numValues = 0;

			// Since a row uses 16 lanes, but we only need to calculate the aggregate for the first half (8) lanes,
			// split the kernel wide aggregation among the first 8 and the second 8 lanes, and then combine them.

			// Initialize the first 8 lanes to the first cell contribution of the kernel.
			// This covers the remainder of 1 in cbLocalMeanVar.KernelWidth / 2 used in the loop below.
            if (GTid4x16.x < GroupDim.x && ShadingConvention::SVGF::IsValidColorValue(value)) {
                valueSum = value;
                squaredValueSum = value * value;
                ++numValues;
            }

			// Get the lane index that has the first value for a kernel in this lane.
            const uint RowKernelStartLaneIndex = RowBaseWaveLaneIndex + 1 // Skip over the already accumulated firt cell of kernel.
				+ (GTid4x16.x < GroupDim.x ? GTid4x16.x : (GTid4x16.x - GroupDim.x) + cbLocalMeanVar.KernelRadius);

            for (uint c = 0; c < cbLocalMeanVar.KernelRadius; ++c) {
                const uint LaneToReadFrom = RowKernelStartLaneIndex + c;
                const float cValue = WaveReadLaneAt(value, LaneToReadFrom);

                if (ShadingConvention::SVGF::IsValidColorValue(cValue)) {
                    valueSum += cValue;
                    squaredValueSum += cValue * cValue;
                    ++numValues;
                }
            }

			// Combine the sub-results.
            const uint LaneToReadFrom = min(WaveGetLaneCount() - 1, RowBaseWaveLaneIndex + GTid4x16.x + GroupDim.x);
            valueSum += WaveReadLaneAt(valueSum, LaneToReadFrom);
            squaredValueSum += WaveReadLaneAt(squaredValueSum, LaneToReadFrom);
            numValues += WaveReadLaneAt(numValues, LaneToReadFrom);

			// Store only the valid results, i.e. first GroupDim columns.
            if (GTid4x16.x < GroupDim.x) {
                PackedRowResultCache[GTid4x16.y][GTid4x16.x] = uint2(ValuePackaging::Float4ToUint(valueSum), ValuePackaging::Float4ToUint(squaredValueSum));
                NumValuesCache[GTid4x16.y][GTid4x16.x] = numValues;
            }
        }
    }
}

void FilterVertically(uint2 DTid, uint2 GTid) {
    float valueSum = 0;
    float squaredValueSum = 0;
    uint numValues = 0;

    const uint2 Pixel = CalcLocalMeanVariance::GetActivePixelIndex(int2(DTid.x, DTid.y * cbLocalMeanVar.PixelStepY));
    
	// Accumulate for the whole kernel.
    for (uint r = 0; r < cbLocalMeanVar.KernelWidth; ++r) {
        const uint RowID = GTid.y + r;
        const uint rNumValues = NumValuesCache[RowID][GTid.x];

        if (rNumValues > 0) {
            const uint2 UnpackedRowSum = PackedRowResultCache[RowID][GTid.x];
            const float rValueSum = UnpackedRowSum.x;
            const float rSquaredValueSum = UnpackedRowSum.y;

            valueSum += rValueSum;
            squaredValueSum += rSquaredValueSum;
            numValues += rNumValues;
        }
    }

	// Calculate mean and variance.
    const float InvN = 1.0 / max(numValues, 1);
    const float Mean = InvN * valueSum;

	// Apply Bessel's correction to the estimated variance, multiply by N/N-1,
	// since the true population mean is not known; it is only estimated as the sample mean.
    const float BesselCorrection = numValues / float(max(numValues, 2) - 1);
    
    float variance = BesselCorrection * (InvN * squaredValueSum - Mean * Mean);
    variance = max(0, variance); // Ensure variance doesn't go negative due to imprecision.

    go_LocalMeanVariance[Pixel] = numValues > 0 ? float2(Mean, variance) : ShadingConvention::SVGF::InvalidContrastValue;
}

[numthreads(
    ShadingConvention::SVGF::ThreadGroup::Default::Width,
    ShadingConvention::SVGF::ThreadGroup::Default::Height,
    ShadingConvention::SVGF::ThreadGroup::Default::Depth)]
void CS(uint2 Gid : SV_GroupID, uint2 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex, uint2 DTid : SV_DispatchThreadID) {
    FilterHorizontally(Gid, GI);
    GroupMemoryBarrierWithGroupSync();

    FilterVertically(DTid, GTid);
}

#endif // __CALCLOCALMEANVARIANCE_HLSL__