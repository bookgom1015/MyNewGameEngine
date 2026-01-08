#ifndef __RAYTRACEDSHADOW_HLSL__
#define __RAYTRACEDSHADOW_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"
#include "./../../../assets/Shaders/HLSL/LightingUtil.hlsli"
#include "./../../../assets/Shaders/HLSL/Shadow.hlsli"
#include "./../../../assets/Shaders/HLSL/Random.hlsli"

typedef BuiltInTriangleIntersectionAttributes Attributes;

struct ShadowHitInfo {
	bool IsHit;
};

ConstantBuffer<ConstantBuffers::LightCB> cbLight : register(b0);

// Nonnumeric values cannot be added to a cbuffer.
RaytracingAccelerationStructure gi_BVH : register(t0);

Texture2D<ShadingConvention::GBuffer::PositionMapFormat>			gi_PositionMap : register(t1);
Texture2D<ShadingConvention::GBuffer::NormalMapFormat>				gi_NormalMap   : register(t2);
Texture2D<ShadingConvention::DepthStencilBuffer::DepthBufferFormat> gi_DepthMap    : register(t3);
RWTexture2D<ShadingConvention::Shadow::ShadowMapFormat>				go_ShadowMap   : register(u0);
   
[shader("raygeneration")]
void RaytracedShadow_RayGen() {
	const uint2 LaunchIndex = DispatchRaysIndex().xy;

	const float3 NormalW = gi_NormalMap[LaunchIndex].xyz;
	const float Depth = gi_DepthMap[LaunchIndex];

	uint value = 0;

	if (Depth != ShadingConvention::DepthStencilBuffer::InvalidDepthValue) {
		const float3 PosW = gi_PositionMap[LaunchIndex].xyz;

		[loop]
		for (uint i = 0; i < cbLight.LightCount; ++i) {
			Render::DX::Foundation::Light light = cbLight.Lights[i];

			if (light.Type == Common::Render::LightType::E_Directional) {
				RayDesc ray;
				ray.Origin = PosW + 0.1f * NormalW;
				ray.Direction = -light.Direction;
				ray.TMin = 0.f;
				ray.TMax = 1000.f;

				ShadowHitInfo payload;
				payload.IsHit = false;

				TraceRay(
					gi_BVH,
					0,
					0xFF,
					0,
					1,
					0,
					ray,
					payload
				);

				value = Shadow::CalcShiftedShadowValueF(
					payload.IsHit ? 0.f : 1.f, value, i);
			}			
			else {
				const float3 Direction = normalize(light.Position - PosW);
				const float Dist = distance(PosW, light.Position);

				RayDesc ray;
				ray.Origin = PosW + 0.1f * Direction;
				ray.Direction = Direction;
				ray.TMin = 0.f;
				ray.TMax = Dist;

				ShadowHitInfo payload;
				payload.IsHit = false;

				TraceRay(
					gi_BVH,
					0,
					0xFF,
					0,
					1,
					0,
					ray,
					payload
				);

				value = Shadow::CalcShiftedShadowValueF(
					payload.IsHit ? 0.f : 1.f, value, i);
			}
		}
	}

	go_ShadowMap[LaunchIndex] = value;
}

[shader("closesthit")]
void RaytracedShadow_ClosestHit(inout ShadowHitInfo payload, Attributes attrib) {
	payload.IsHit = true;
}

[shader("miss")]
void RaytracedShadow_Miss(inout ShadowHitInfo payload) {
	payload.IsHit = false;
}

#endif // __RAYTRACEDSHADOW_HLSL__