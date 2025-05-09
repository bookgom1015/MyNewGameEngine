#pragma once

#ifdef _HLSL
	#include "./../../../../inc/Render/DX/Foundation/Light.h"
#else
	#include "Render/DX/Foundation/Light.h"
#endif

namespace ConstantBuffers {
	struct PassCB {
		DirectX::XMFLOAT4X4	View;
		DirectX::XMFLOAT4X4	InvView;
		DirectX::XMFLOAT4X4	Proj;
		DirectX::XMFLOAT4X4	InvProj;
		DirectX::XMFLOAT4X4	ViewProj;
		DirectX::XMFLOAT4X4	InvViewProj;
		DirectX::XMFLOAT4X4 PrevViewProj;
		DirectX::XMFLOAT4X4 ViewProjTex;

		DirectX::XMFLOAT3	EyePosW;
		FLOAT				__ConstantPad0__;

		DirectX::XMFLOAT2	JitteredOffset;
		FLOAT				__ConstantPad1__;
		FLOAT				__ConstantPad2__;
	};

	struct LightCB {
		UINT	LightCount;
		FLOAT	__ConstantPad0__;
		FLOAT	__ConstantPad1__;
		FLOAT	__ConstantPad2__;

		Render::DX::Foundation::Light Lights[MaxLights];
	};

	struct ObjectCB {
		DirectX::XMFLOAT4X4 World;
		DirectX::XMFLOAT4X4 PrevWorld;
		DirectX::XMFLOAT4X4 TexTransform;
		DirectX::XMFLOAT4	Center;
		DirectX::XMFLOAT4	Extents;
	};

	struct MaterialCB {
		DirectX::XMFLOAT4	Albedo;

		FLOAT				Roughness;
		FLOAT				Metalness;
		FLOAT				__ConstantPad0__;
		FLOAT				__ConstantPad1__;

		DirectX::XMFLOAT3	Specular;
		FLOAT				__ConstantPad2__;

		DirectX::XMFLOAT4X4	MatTransform;

		INT					AlbedoMapIndex;
		INT					NormalMapIndex;
		INT					AlphaMapIndex;
		INT					RoughnessMapIndex;

		INT					MetalnessMapIndex;
		INT					SpecularMapIndex;
		FLOAT				__ConstantPad3__;
		FLOAT				__ConstantPad4__;
	};

	struct ProjectToCubeCB {
		DirectX::XMFLOAT4X4 Proj;
		DirectX::XMFLOAT4X4 View[6];
	};

	struct SsaoCB {
		DirectX::XMFLOAT4X4	View;
		DirectX::XMFLOAT4X4	Proj;
		DirectX::XMFLOAT4X4	InvProj;
		DirectX::XMFLOAT4X4	ProjTex;
		DirectX::XMFLOAT4	OffsetVectors[14];

		FLOAT				OcclusionRadius;
		FLOAT				OcclusionFadeStart;
		FLOAT				OcclusionFadeEnd;
		FLOAT				SurfaceEpsilon;

		UINT				SampleCount;
		FLOAT				__ConstantPad0__;
		FLOAT				__ConstantPad1__;
		FLOAT				__ConstantPad2__;
	};
}