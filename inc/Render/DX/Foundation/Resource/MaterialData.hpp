#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <Windows.h>

#include "Common/Util/MathUtil.hpp"

namespace Render::DX::Foundation::Resource {
	struct MaterialData {
		MaterialData(INT numFrameResource);

		INT MaterialCBIndex = -1;

		INT AlbedoMapIndex = -1;
		INT NormalMapIndex = -1;
		INT AlphaMapIndex = -1;
		INT RoughnessMapIndex = -1;
		INT MetalnessMapIndex = -1;
		INT SpecularMapIndex = -1;

		// Dirty flag indicating the material has changed and we need to update the constant buffer.
		// Because we have a material constant buffer for each FrameResource, we have to apply the
		// update to each FrameResource.  Thus, when we modify a material we should set 
		// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
		INT NumFramesDirty;

		// Material constant buffer data used for shading.
		DirectX::XMFLOAT4 Albedo = { 1.f, 1.f, 1.f, 1.f };
		DirectX::XMFLOAT3 Specular = { 0.08f, 0.08f, 0.08f };
		FLOAT Roughness = 0.5f;
		FLOAT Metalness = 0.f;

		DirectX::XMFLOAT4X4 MatTransform = Common::Util::MathUtil::Identity4x4();
	};
}