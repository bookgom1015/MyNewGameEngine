#pragma once

#ifdef _HLSL
	struct Material {
		float4 Albedo;
		float3 FresnelR0;
		float  Shininess;
		float  Metalic;
	};
#else
	#include <string>

	#include <Windows.h>

	#include <DirectXMath.h>
#endif // _HLSL

namespace Common {
	namespace Foundation {
		namespace Mesh {
#ifndef _HLSL
			struct Material {
				std::string Name;

				std::string DiffuseMap;
				DirectX::XMFLOAT4 Albedo = { 1.f, 1.f, 1.f, 1.f };

				std::string NormalMap;

				std::string AlphaMap;
				FLOAT Alpha = 1.f;

				std::string RoughnessMap;
				FLOAT Roughness = 0.5f;

				std::string MetalnessMap;
				FLOAT Metalness = 0.f;

				std::string SpecularMap;
				DirectX::XMFLOAT3 Specular = { 0.08f, 0.08f, 0.08f };
			};
#endif // _HLSL
		}	
	}
}