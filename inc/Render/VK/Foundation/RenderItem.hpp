#pragma once

namespace Render::VK::Foundation {
	struct RenderItem {
		RenderItem(std::uint32_t numSwapChainImages);

		std::vector<VkDescriptorSet> DescriptorSets{};

		std::vector<VkBuffer>		UniformBuffers{};
		std::vector<VkDeviceMemory>	UniformBufferMemories{};

		std::string MeshName{};
		std::string MatName{};

		DirectX::XMVECTOR Scale{};
		DirectX::XMVECTOR Rotation{};
		DirectX::XMVECTOR Position{};

		std::uint32_t NumFramesDirty{};
	};
}