#ifndef __BLENDSCATTERING_HLSL__
#define __BLENDSCATTERING_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"

Texture3D<ShadingConvention::VolumetricLight::FrustumVolumeMapFormat> gi_PreviousFrame : register(t0);

RWTexture3D<ShadingConvention::VolumetricLight::FrustumVolumeMapFormat> gio_CurrentFrame : register(u0);

[numthreads(
	ShadingConvention::VolumetricLight::ThreadGroup::BlendScattering::Width, 
	ShadingConvention::VolumetricLight::ThreadGroup::BlendScattering::Height, 
	ShadingConvention::VolumetricLight::ThreadGroup::BlendScattering::Depth)]
void CS(in uint3 DTid : SV_DispatchThreadId) {
	const float4 CurrScattering = gio_CurrentFrame[DTid];	
	const float4 PrevScattering = gi_PreviousFrame[DTid];
		
	float4 scattering = CurrScattering;
	
	const float4 Diff = PrevScattering - CurrScattering;
	if (!any(Diff > 0.01f)) scattering = lerp(PrevScattering, CurrScattering, 0.8f);		
	
	gio_CurrentFrame[DTid] = scattering;
}

#endif // __BLENDSCATTERING_HLSL__