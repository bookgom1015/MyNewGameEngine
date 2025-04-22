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
		UINT				LightCount;

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
		FLOAT				Metalic;
		FLOAT				Specular;
		FLOAT				ConstantPad0;

		DirectX::XMFLOAT4X4	MatTransform;

		INT					DiffuseSrvIndex;
		INT					NormalSrvIndex;
		INT					AlphaSrvIndex;
		FLOAT				MatConstPad0;
	};

	struct EquirectangularConverterCB {
		DirectX::XMFLOAT4X4 Proj;
		DirectX::XMFLOAT4X4 View[6];
	};
}