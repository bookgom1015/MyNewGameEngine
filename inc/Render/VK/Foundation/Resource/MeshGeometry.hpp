#pragma once

#include <cstdint>

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

#include "Common/Util/HashUtil.hpp"

namespace Render::VK::Foundation::Resource {
	struct MeshGeometry {
		VkBuffer		VertexBuffer;
		VkDeviceMemory	VertexBufferMemory;

		VkBuffer		IndexBuffer;
		VkDeviceMemory	IndexBufferMemory;

		std::uint32_t	IndexCount;

		static Common::Foundation::Hash Hash(MeshGeometry* ptr);
	};
}