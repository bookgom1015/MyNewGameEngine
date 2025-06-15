#ifndef __GAUSSIANBLURFILTER3X3CS_HLSL
#define __GAUSSIANBLURFILTER3X3CS_HLSL

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"

BlurFilter_Default_RootConstants(b0)

Texture2D<float>   gi_InputMap  : register(t0);
RWTexture2D<float> go_OutputMap : register(u0);

static const float Weights[3][3] = {
    { 0.077847f, 0.123317f, 0.077847f },
    { 0.123317f, 0.195346f, 0.123317f },
    { 0.077847f, 0.123317f, 0.077847f },
};

#define APPROXIMATE_GAUSSIAN_3X3_VIA_HW_FILTERING 1

#if APPROXIMATE_GAUSSIAN_3X3_VIA_HW_FILTERING
// Approximate 3x3 gaussian filter using HW bilinear filtering.
// Ref: Moller2018, Real-Time Rendering (Fourth Edition), p517
// Performance improvement over 3x3 2D version (4K on 2080 Ti): 0.18ms -> 0.11ms
[numthreads(
    ShadingConvention::BlurFilter::ThreadGroup::Default::Width,
    ShadingConvention::BlurFilter::ThreadGroup::Default::Height,
    ShadingConvention::BlurFilter::ThreadGroup::Default::Depth)]
void CS(in uint2 dispatchThreadID : SV_DispatchThreadID) {
	// Set weights based on availability of neightbor samples.
    float4 weights;

    const uint2 border = uint2(gTexDim.x - 1, gTexDim.y - 1);

	// Non-border pixels
    if (dispatchThreadID.x > 0 && dispatchThreadID.y > 0 && dispatchThreadID.x < border.x && dispatchThreadID.x < border.y) {
        weights = float4(0.077847f + 0.123317f + 0.123317f + 0.195346f,
						 0.077847f + 0.123317f,
						 0.077847f + 0.123317f,
						 0.077847f);
    }
	// Top-left corner
    else if (dispatchThreadID.x == 0 && dispatchThreadID.y == 0) {
        weights = float4(0.195346f, 0.123317f, 0.123317f, 0.077847f) / 0.519827f;
    }
	// Top-right corner
    else if (dispatchThreadID.x == border.x && dispatchThreadID.y == 0) {
        weights = float4(0.123317f + 0.195346f, 0.f, 0.201164f, 0.f) / 0.519827f;
    }
	// Bootom-left corner
    else if (dispatchThreadID.x == 0 && dispatchThreadID.y == border.y) {
        weights = float4(0.123317f + 0.195346f, 0.077847f + 0.123317f, 0.f, 0.f) / 0.519827f;
    }
	// Bottom-right corner
    else if (dispatchThreadID.x == border.x && dispatchThreadID.y == border.y) {
        weights = float4(0.077847f + 0.123317f + 0.123317f + 0.195346f, 0.f, 0.f, 0.f) / 0.519827f;
    }
	// Left border
    else if (dispatchThreadID.x == 0) {
        weights = float4(0.123317f + 0.195346f, 0.077847f + 0.123317f, 0.123317f, 0.077847f) / 0.720991f;
    }
	// Right border
    else if (dispatchThreadID.x == border.x) {
        weights = float4(0.077847f + 0.123317f + 0.123317f + 0.195346f, 0.f, 0.077847f + 0.123317f, 0.f) / 0.720991f;
    }
	// Top border
    else if (dispatchThreadID.y == 0) {
        weights = float4(0.123317f + 0.195346f, 0.123317f, 0.077847f + 0.123317f, 0.077847f) / 0.720991f;
    }
	// Bottom border
    else {
        weights = float4(0.077847f + 0.123317f + 0.123317f + 0.195346f, 0.077847f + 0.123317f, 0.f, 0.f) / 0.720991f;
    }

    const float2 offsets[3] = {
        float2(0.5f, 0.5f) + float2(-0.123317f / (0.123317f + 0.195346f), -0.123317f / (0.123317f + 0.195346f)),
		float2(0.5f, 0.5f) + float2(1.f, -0.077847f / (0.077847f + 0.123317f)),
		float2(0.5f, 0.5f) + float2(-0.077847f / (0.077847f + 0.123317f), 1.f)
    };

    const float4 samples = float4(
		gi_InputMap.SampleLevel(gsamLinearMirror, (dispatchThreadID + offsets[0]) * gInvTexDim, 0),
		gi_InputMap.SampleLevel(gsamLinearMirror, (dispatchThreadID + offsets[1]) * gInvTexDim, 0),
		gi_InputMap.SampleLevel(gsamLinearMirror, (dispatchThreadID + offsets[2]) * gInvTexDim, 0),
		gi_InputMap[dispatchThreadID + 1]
	);

    go_OutputMap[dispatchThreadID] = dot(samples, weights);
}
#else 
void AddFilterContribution(inout float weightedValueSum, inout float weightSum, in int row, in int col, in int2 dispatchThreadID) {
	int2 id = dispatchThreadID + int2(row - 1, col - 1);
	if (id.x >= 0 && id.y >= 0 && id.x < gTexDim.x && id.y < gTexDim.y) {
		float weight = Weights[col][row];
		weightedValueSum += weight * gi_InputMap[id];
		weightSum += weight;
	}
}

[numthreads(
    ShadingConvention::BlurFilter::ThreadGroup::Default::Width, 
    ShadingConvention::BlurFilter::ThreadGroup::Default::Height, 
    ShadingConvention::BlurFilter::ThreadGroup::Default::Depth)]
void CS(uint2 dispatchThreadID : SV_DispatchThreadID) {
	float weightSum = 0.f;
	float weightedValueSum = 0.f;

	[unroll]
	for (uint r = 0; r < 3; ++r) {
		[unroll]
		for (uint c = 0; c < 3; ++c) {
			AddFilterContribution(weightedValueSum, weightSum, r, c, dispatchThreadID);
		}
	}

	go_OutputMap[dispatchThreadID] = weightedValueSum / weightSum;
}
#endif // APPROXIMATE_GAUSSIAN_3X3_VIA_HW_FILTERING
#endif // __GAUSSIANBLURFILTER3X3CS_HLSL