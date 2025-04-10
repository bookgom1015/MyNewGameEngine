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

	struct EquirectangularConverterCB {
		DirectX::XMFLOAT4X4 Proj;
		DirectX::XMFLOAT4X4 View[6];
	};
}