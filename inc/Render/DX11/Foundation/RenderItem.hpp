#pragma once

#include "Common/Util/MathUtil.hpp"
#include "Common/Util/HashUtil.hpp"

namespace Render::DX11::Foundation {
	namespace Resource {
		class MeshGeometry;
		struct MaterialData;
	}

	struct RenderItem {		
		INT ObjectCBIndex{ -1 };

		DirectX::XMFLOAT4X4 World = Common::Util::MathUtil::Identity4x4();
		DirectX::XMFLOAT4X4 PrevWorld = Common::Util::MathUtil::Identity4x4();
		DirectX::XMFLOAT4X4 TexTransform = Common::Util::MathUtil::Identity4x4();

		INT FrameDirty{ TRUE };

		Resource::MaterialData* Material{};
		Resource::MeshGeometry* Geometry{};

		UINT IndexCount{};
		UINT StartIndexLocation{};
		UINT BaseVertexLocation{};

		D3D11_PRIMITIVE_TOPOLOGY PrimitiveType{ D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST };


	public:
		static Common::Foundation::Hash Hash(const RenderItem* ptr);
	};
}