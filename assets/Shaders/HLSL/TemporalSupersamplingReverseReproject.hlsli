#ifndef __TEMPORALSUPERSAMPLINGREVERSEREPROJECT_HLSLI__
#define __TEMPORALSUPERSAMPLINGREVERSEREPROJECT_HLSLI__

namespace TemporalSupersamplingReverseReproject {
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
}

#endif // __TEMPORALREPROJECT_HLSLI__