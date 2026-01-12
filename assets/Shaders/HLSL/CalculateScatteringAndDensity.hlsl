#ifndef __CALCULATESCATTERINGANDDENSITY_HLSL__
#define __CALCULATESCATTERINGANDDENSITY_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"
#include "./../../../assets/Shaders/HLSL/LightingUtil.hlsli"
#include "./../../../assets/Shaders/HLSL/Shadow.hlsli"
#include "./../../../assets/Shaders/HLSL/Random.hlsli"
#include "./../../../assets/Shaders/HLSL/VolumetricLight.hlsli"

ConstantBuffer<ConstantBuffers::PassCB> cbPass		: register(b0);
ConstantBuffer<ConstantBuffers::LightCB> cbLight	: register(b1);

VolumetricLight_CalculateScatteringAndDensity_RootConstants(b2)

Texture2D<ShadingConvention::Shadow::ZDepthMapFormat>				    gi_ZDepthMaps[MaxLights]		: register(t0);
Texture2DArray<ShadingConvention::Shadow::ZDepthMapFormat>			    gi_ZDepthCubeMaps[MaxLights]	: register(t0, space1);

RWTexture3D<ShadingConvention::VolumetricLight::FrustumVolumeMapFormat>	go_FrustumVolumeMap			    : register(u0);

[numthreads(
	ShadingConvention::VolumetricLight::ThreadGroup::CalculateScatteringAndDensity::Width, 
	ShadingConvention::VolumetricLight::ThreadGroup::CalculateScatteringAndDensity::Height, 
	ShadingConvention::VolumetricLight::ThreadGroup::CalculateScatteringAndDensity::Depth)]
void CS(in uint3 DTid : SV_DispatchThreadId) {
	uint3 dims;
	go_FrustumVolumeMap.GetDimensions(dims.x, dims.y, dims.z);
	if (any(DTid >= dims)) return;
	
	const uint Idx = Random::Hash3D(DTid) + gFrameCount;
	const float Jitter = Random::HaltonSequence[Idx % MAX_HALTON_SEQUENCE].z - 0.5f;

	const float3 PosW = ShaderUtil::ThreadIdToWorldPosition(
		float3((float2)DTid.xy, (float)DTid.z + Jitter), 
			dims, gDepthExponent, gNearZ, gFarZ, cbPass.InvView, cbPass.InvProj);
	const float3 ToEyeW = normalize(cbPass.EyePosW - PosW);

	float3 Li = 0.f; // Ambient lights;

	[loop]
	for (uint i = 0; i < cbLight.LightCount; ++i) {
		Render::DX::Foundation::Light light = cbLight.Lights[i];

		float3 direction = 0.f;
		float Ld = 0.f;
		float falloff = 1.f;

		if (light.Type == Common::Render::LightType::E_Directional) {
			direction = light.Direction;
		}
		else if (light.Type == Common::Render::LightType::E_Tube || light.Type == Common::Render::LightType::E_Rect) {
			// Tube and rectangular light do not have shadow(depth) map for now, 
			//  so these can not calculate visibility.
			continue;
		}
		else {
			direction = PosW - light.Position;
			Ld = length(direction);
		}
		
		float visibility = 1.f;
		if (light.Type == Common::Render::LightType::E_Point || light.Type == Common::Render::LightType::E_Tube) {
			const uint Index = ShaderUtil::GetCubeFaceIndex(direction);
			const float3 Normalized = normalize(direction);
			const float2 UV = ShaderUtil::ConvertDirectionToUV(Normalized);
			
			const float4x4 ViewProj = Shadow::GetViewProjMatrix(light, Index);
			
			if (any(ViewProj != 0.f)) visibility = Shadow::CalcShadowFactorCube(gi_ZDepthCubeMaps[i], gsamShadow, ViewProj, PosW.xyz, UV, Index);

			falloff = CalcInverseSquareAttenuation(Ld, light.AttenuationRadius);
		}
		else if (light.Type == Common::Render::LightType::E_Directional || light.Type == Common::Render::LightType::E_Spot) {			
			visibility = Shadow::CalcShadowFactor(gi_ZDepthMaps[i], gsamShadow, light.Mat1, PosW);
		}

		const float PhaseFunction = VolumetricLight::HenyeyGreensteinPhaseFunction(direction, ToEyeW, gAnisotropicCoefficient);

		Li += visibility * light.Color * light.Intensity * falloff * PhaseFunction;
	}
	
	go_FrustumVolumeMap[DTid] = float4(Li * gUniformDensity, gUniformDensity);
}

#endif // __CALCULATESCATTERINGANDDENSITY_HLSL__