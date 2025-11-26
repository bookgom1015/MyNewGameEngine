#ifndef __EXTRACTHIGHLIGHTS_HLSL__
#define __EXTRACTHIGHLIGHTS_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef _FIT_TO_SCREEN_COORD
#define _FIT_TO_SCREEN_COORD
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"

Texture2D<ShadingConvention::ToneMapping::IntermediateMapFormat> gi_BackBuffer : register(t0);

Bloom_ExtractHighlights_RootConstants(b0)

FitToScreenVertexOut

FitToScreenVertexShader

FitToScreenMeshShader

ShadingConvention::Bloom::HighlightMapFormat PS(VertexOut pin) : SV_TARGET {
    const float3 HDR = gi_BackBuffer.Sample(gsamLinearWrap, pin.TexC).rgb;
	const float Luminance = dot(HDR, float3(0.2126f, 0.7152f, 0.0722f));
	const float Mask = step(gThreshold, Luminance);
	const float3 Brightness = HDR * Mask;

	return float4(Brightness, 1.f);
}

#endif // __EXTRACTHIGHLIGHTS_HLSL__