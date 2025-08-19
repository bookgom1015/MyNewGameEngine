#ifndef __APPLYFOG_HLSL__
#define __APPLYFOG_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef _FIT_TO_SCREEN_COORD
#define _FIT_TO_SCREEN_COORD
#endif

#include "./../../../inc/Render/DX/Foundation/HlslCompaction.h"
#include "./../../../assets/Shaders/HLSL/Samplers.hlsli"

ConstantBuffer<ConstantBuffers::PassCB> cbPass : register(b0);

Texture2D<ShadingConvention::GBuffer::PositionMapFormat>				gi_PositionMap		: register(t0);
Texture3D<ShadingConvention::VolumetricLight::FrustumVolumeMapFormat>	gi_FrustumVolumeMap	: register(t1);

struct VertexOut {
	float4 PosH		: SV_POSITION;
	float2 TexC		: TEXCOORD;
};

struct PixelOut {
	HDR_FORMAT	OutputMap	: SV_TARGET0;
	//VolumetricLight::DebugMapFormat DebugMap : SV_TARGET1;
};

FitToScreenVertexShader

FitToScreenMeshShader

PixelOut PS(VertexOut pin) {
	uint3 dims;
	gi_FrustumVolumeMap.GetDimensions(dims.x, dims.y, dims.z);

	const float4 PosW = gi_PositionMap.SampleLevel(gsamLinearClamp, pin.TexC, 0);
	
	float4 posV = mul(PosW, cbPass.View);
	if (!ShadingConvention::GBuffer::IsValidPosition(PosW)) posV.z = dims.z - 1;
	
	const float3 TexC = float3(pin.TexC, min(posV.z / dims.z, 1.f));

	const float4 ScatteringAndTransmittance = gi_FrustumVolumeMap.SampleLevel(gsamLinearClamp, TexC, 0);
	const float3 ScatteringColor = ScatteringAndTransmittance.rgb;

	PixelOut pout = (PixelOut)0;
	pout.OutputMap = float4(ScatteringColor, ScatteringAndTransmittance.a);
	//pout.DebugMap = float4(TexC, 0.f);
	
	return pout;
}

#endif // __APPLYFOG_HLSL__