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
			BOOL InitializeVulkan();
			void CleanUpVulkan();

		private:
			BOOL mbIsVulkanInitialized{};
		};
	}
}