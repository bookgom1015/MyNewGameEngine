#ifndef __BLENDSCATTERING_HLSL__
#define __BLENDSCATTERING_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"

Texture3D<ShadingConvention::VolumetricLight::FrustumVolumeMapFormat> gi_PreviousFrame : register(t0);

RWTexture3D<ShadingConvention::VolumetricLight::FrustumVolumeMapFormat> gio_CurrentFrame : register(u0);

float4 ClampToNeighborhood(in float4 prev, in float4 curr, in float k) {
    float4 lo = curr - k;
    float4 hi = curr + k;
    return clamp(prev, lo, hi);
}																																																																						

[numthreads(
	ShadingConvention::VolumetricLight::ThreadGroup::BlendScattering::Width, 
	ShadingConvention::VolumetricLight::ThreadGroup::BlendScattering::Height, 
	ShadingConvention::VolumetricLight::ThreadGroup::BlendScattering::Depth)]
void CS(in uint3 DTid : SV_DispatchThreadId) {
	const float4 CurrScattering = gio_CurrentFrame[DTid];	
	float4 prevScattering = gi_PreviousFrame[DTid];
																																																																							
	const float4 Delta = abs(CurrScattering - prevScattering);
	
	float diff = max(max(Delta.x, Delta.y), Delta.z);
	diff = max(diff, Delta.w);
																																																																							
	const float Scale = max(1e-3f, max(max(CurrScattering.x, CurrScattering.y), CurrScattering.z));
	float k = 0.15f * Scale + 0.01f;
	prevScattering = clamp(prevScattering , CurrScattering - k, CurrScattering + k);
		
	const float a = smoothstep(0.02f, 0.15f, diff);
	const float AlphaRGB = lerp(0.05f, 0.9f, a);
	const float AlphaA = 0.02f;
		
	const float3 ResultRGB = lerp(prevScattering.rgb, CurrScattering.rgb, AlphaRGB);
	const float ResultA = lerp(prevScattering.a, CurrScattering.a, AlphaA);
    gio_CurrentFrame[DTid] = float4(ResultRGB, ResultA);
}

#endif // __BLENDSCATTERING_HLSL__