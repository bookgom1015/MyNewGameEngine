#pragma once

#pragma comment(lib, "vulkan-1.lib")

#include "Common/ImGuiManager/ImGuiManager.hpp"

#include <memory>

namespace ImGuiManager {
	extern "C" ImGuiManagerAPI Common::ImGuiManager::ImGuiManager* CreateImGuiManager();
	extern "C" ImGuiManagerAPI void DestroyImGuiManager(Common::ImGuiManager::ImGuiManager* const imGuiManager);

	namespace VK {
		class VkImGuiManager : public Common::ImGuiManager::ImGuiManager {
		public:
			VkImGuiManager() = default;
			virtual ~VkImGuiManager() = default;

		public:
			BOOL InitializeVulkan();
			void CleanUpVulkan();

		private:
			BOOL mbIsVulkanInitialized = FALSE;
		};
	}
}