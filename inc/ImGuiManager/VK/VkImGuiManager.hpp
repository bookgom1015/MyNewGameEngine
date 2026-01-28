#pragma once

#include "Common/ImGuiManager/ImGuiManager.hpp"

namespace ImGuiManager {
	extern "C" ImGuiManagerAPI Common::ImGuiManager::ImGuiManager* CreateImGuiManager();
	extern "C" ImGuiManagerAPI void DestroyImGuiManager(Common::ImGuiManager::ImGuiManager* const imGuiManager);

	namespace VK {
		class VkImGuiManager : public Common::ImGuiManager::ImGuiManager {
		public:
			VkImGuiManager();
			virtual ~VkImGuiManager();

		public:
			ImGuiManagerAPI BOOL InitializeVulkan(
				const VkInstance& instance,
				const VkPhysicalDevice& physicalDevice,
				const VkDevice& device,
				const VkQueue& graphicsQueue,
				const VkDescriptorPool& descriptorPool,
				VkFormat swapchainImageFormat,
				std::uint32_t minImageCount,
				std::uint32_t imageCount);
			ImGuiManagerAPI virtual void CleanUp() override;

		private:
			BOOL mbIsVulkanInitialized{};
		};
	}
}