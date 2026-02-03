#pragma once

#include "Common/Util/MathUtil.hpp"

namespace Render::DX11::Foundation::Resource {
	struct MaterialData {
		MaterialData(UINT numFrameResources);

		INT MaterialCBIndex{ -1 };

		INT AlbedoMapIndex{ -1 };
		INT NormalMapIndex{ -1 };
		INT AlphaMapIndex{ -1 };
		INT RoughnessMapIndex{ -1 };
		INT MetalnessMapIndex{ -1 };
		INT SpecularMapIndex{ -1 };

		INT FrameDirty{};

		DirectX::XMFLOAT4 Albedo{ 1.f, 1.f, 1.f, 1.f };
		DirectX::XMFLOAT3 Specular{ 0.08f, 0.08f, 0.08f };
		FLOAT Roughness{ 0.5f };
		FLOAT Metalness{ 0.f };

		DirectX::XMFLOAT4X4 MatTransform = Common::Util::MathUtil::Identity4x4();
	};
}