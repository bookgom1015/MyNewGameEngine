#pragma once

#include "Common/Util/HashUtil.hpp"

namespace Render::VK::Foundation::Resource {
	struct MeshGeometry {
		VkBuffer		VertexBuffer{};
		VkDeviceMemory	VertexBufferMemory{};

		VkBuffer		IndexBuffer{};
		VkDeviceMemory	IndexBufferMemory{};

		std::uint32_t	IndexCount{};

		static Common::Foundation::Hash Hash(MeshGeometry* ptr);
	};
}