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

Texture3D<ShadingConvention::VolumetricLight::FrustumVolumeMapFormat>   gi_PrevFrustumVolumeMap			: register(t0);
Texture2D<ShadingConvention::Shadow::ZDepthMapFormat>				    gi_ZDepthMaps[MaxLights]		: register(t1);
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
	
	const float3 Jitter = (Random::HaltonSequence[(gFrameCount + DTid.x + DTid.y * 2) % MAX_HALTON_SEQUENCE] - (float3)0.5f) * 0.25f;

	const float3 NotJitteredPosW = ShaderUtil::ThreadIdToWorldPosition(DTid, dims, gDepthExponent, gNearZ, gFarZ, cbPass.InvView, cbPass.InvProj);
	const float3 PosW = NotJitteredPosW + Jitter;
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
			// Tube and rectangular light do not have shadow(depth) map, 
			//  so these can not calculate visibility.
			return;
		}
		else {
			direction = PosW - light.Position;
			Ld = length(direction);
		}
		
		//const float3 ToLight = -direction;
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

	{
		const float4 CurrScattering = float4(Li * gUniformDensity, gUniformDensity);
		const float3 PrevFrameUV = VolumetricLight::ConvertPositionToUV(NotJitteredPosW, cbPass.PrevViewProj);
	
		float4 scattering = CurrScattering;
		
		//if (all(PrevFrameUV <= (float3)1.f) && all(PrevFrameUV >= (float3)0.f)) {
		//	const float4 PrevScattering = gi_PrevFrustumVolumeMap.SampleLevel(gsamLinearClamp, PrevFrameUV, 0);
		//
		//	if (any(PrevScattering != 0.f)) scattering = lerp(PrevScattering, CurrScattering, 0.05f);
		//}
		
		go_FrustumVolumeMap[DTid] = scattering;
	}
}

#endif // __CALCULATESCATTERINGANDDENSITY_HLSL__