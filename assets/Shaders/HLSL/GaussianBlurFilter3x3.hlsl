#ifndef __GAUSSIANBLURFILTER3X3_HLSL
#define __GAUSSIANBLURFILTER3X3_HLSL

#ifndef _HLSL
#define _HLSL
#endif

#ifndef ValueType
#define ValueType float
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"

BlurFilter_Default_RootConstants(b0)

Texture2D<ValueType>   gi_InputMap  : register(t0);
RWTexture2D<ValueType> go_OutputMap : register(u0);

static const float Weights[3][3] = {
    { 0.077847f, 0.123317f, 0.077847f },
    { 0.123317f, 0.195346f, 0.123317f },
    { 0.077847f, 0.123317f, 0.077847f },
};

// Approximate 3x3 gaussian filter using HW bilinear filtering.
// Ref: Moller2018, Real-Time Rendering (Fourth Edition), p517
// Performance improvement over 3x3 2D version (4K on 2080 Ti): 0.18ms -> 0.11ms
[numthreads(
    ShadingConvention::BlurFilter::ThreadGroup::Default::Width,
    ShadingConvention::BlurFilter::ThreadGroup::Default::Height,
    ShadingConvention::BlurFilter::ThreadGroup::Default::Depth)]
void CS(in uint2 DTid : SV_DispatchThreadID) {
	// Set weights based on availability of neightbor samples.
    float4 weights;

    const uint2 border = uint2(gTexDim.x - 1, gTexDim.y - 1);

	// Non-border pixels
    if (DTid.x > 0 && DTid.y > 0 && DTid.x < border.x && DTid.x < border.y) {
        weights = float4(0.077847f + 0.123317f + 0.123317f + 0.195346f,
						 0.077847f + 0.123317f,
						 0.077847f + 0.123317f,
						 0.077847f);
    }
	// Top-left corner
    else if (DTid.x == 0 && DTid.y == 0) {
        weights = float4(0.195346f, 0.123317f, 0.123317f, 0.077847f) / 0.519827f;
    }
	// Top-right corner
    else if (DTid.x == border.x && DTid.y == 0) {
        weights = float4(0.123317f + 0.195346f, 0.f, 0.201164f, 0.f) / 0.519827f;
    }
	// Bootom-left corner
    else if (DTid.x == 0 && DTid.y == border.y) {
        weights = float4(0.123317f + 0.195346f, 0.077847f + 0.123317f, 0.f, 0.f) / 0.519827f;
    }
	// Bottom-right corner
    else if (DTid.x == border.x && DTid.y == border.y) {
        weights = float4(0.077847f + 0.123317f + 0.123317f + 0.195346f, 0.f, 0.f, 0.f) / 0.519827f;
    }
	// Left border
    else if (DTid.x == 0) {
        weights = float4(0.123317f + 0.195346f, 0.077847f + 0.123317f, 0.123317f, 0.077847f) / 0.720991f;
    }
	// Right border
    else if (DTid.x == border.x) {
        weights = float4(0.077847f + 0.123317f + 0.123317f + 0.195346f, 0.f, 0.077847f + 0.123317f, 0.f) / 0.720991f;
    }
	// Top border
    else if (DTid.y == 0) {
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

    ValueType samples[4];                                                                               
    samples[3] = gi_InputMap[DTid + 1];
    
    [unroll]
    for (uint i = 0; i < 3; ++i) {
        samples[i] = gi_InputMap.SampleLevel(gsamLinearMirror, (DTid + offsets[i]) * gInvTexDim, 0);
    }
    
    ValueType result = (ValueType)0.f;
    [unroll]
    for (uint i = 0; i < 4; ++i) {
        result += samples[i] * weights[i];
    }

    go_OutputMap[DTid] = result;
}

#endif // __GAUSSIANBLURFILTER3X3_HLSL