#ifndef __APPLYBLOOM_HLSL__
#define __APPLYBLOOM_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef _FIT_TO_SCREEN_COORD
#define _FIT_TO_SCREEN_COORD
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"

Texture2D<ShadingConvention::ToneMapping::IntermediateMapFormat>	gi_BackBuffer	: register(t0);
Texture2D<ShadingConvention::Bloom::HighlightMapFormat>				gi_BloomMap		: register(t1);

Bloom_ExtractHighlights_RootConstants(b0)

FitToScreenVertexOut

FitToScreenVertexShader

FitToScreenMeshShader

float3 SoftAddBloom(in float3 hdr, in float3 bloom) {
	return lerp(hdr, hdr + bloom, 0.7f);
}

float3 ToneAwareBloom(in float3 hdr, in float3 bloom) {
	const float3 blended = hdr + bloom;
	const float luminance = dot(hdr, float3(0.2126f, 0.7152f, 0.0722f));
	
	return lerp(blended, bloom, 1 - exp(-luminance));
}

ShadingConvention::ToneMapping::IntermediateMapFormat PS(VertexOut pin) : SV_TARGET {
    const float3 Scene = gi_BackBuffer.SampleLevel(gsamLinearClamp, pin.TexC, 0).rgb;
	const float3 Bloom = gi_BloomMap.SampleLevel(gsamLinearClamp, pin.TexC, 0).rgb;
	
	const float3 Color = SoftAddBloom(Scene, Bloom);
	
	return float4(Color, 1.f);
}

#endif // __APPLYBLOOM_HLSL__