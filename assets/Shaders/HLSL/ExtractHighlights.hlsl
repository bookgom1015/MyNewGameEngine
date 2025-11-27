#ifndef __EXTRACTHIGHLIGHTS_HLSL__
#define __EXTRACTHIGHLIGHTS_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"

RWTexture2D<ShadingConvention::Bloom::HighlightMapFormat> uio_HighlightMap : register(u0);

Bloom_ExtractHighlights_RootConstants(b0)

[numthreads(
    ShadingConvention::Bloom::ThreadGroup::Default::Width,
    ShadingConvention::Bloom::ThreadGroup::Default::Height,
    ShadingConvention::Bloom::ThreadGroup::Default::Depth)]
void CS(in uint2 DTid : SV_DispatchThreadID) {
    const float3 HDR = uio_HighlightMap[DTid].rgb;
	const float Luminance = dot(HDR, float3(0.2126f, 0.7152f, 0.0722f));
	const float Mask = step(gThreshold, Luminance);
	const float3 Brightness = HDR * Mask;

	uio_HighlightMap[DTid] = float4(Brightness, 1.f);
}

#endif // __EXTRACTHIGHLIGHTS_HLSL__