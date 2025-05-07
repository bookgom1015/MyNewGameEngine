#include "ImGuiManager/VK/VkImGuiManager.hpp"
#include "Common/Debug/Logger.hpp"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_vulkan.h>

using namespace ImGuiManager::VK;

extern "C" ImGuiManagerAPI Common::ImGuiManager::ImGuiManager* ImGuiManager::CreateImGuiManager() {
	return new VkImGuiManager();
}

extern "C" ImGuiManagerAPI void ImGuiManager::DestroyImGuiManager(Common::ImGuiManager::ImGuiManager* const imGuiManager) {
	delete imGuiManager;
}

BOOL VkImGuiManager::InitializeVulkan() {
	ImGui_ImplVulkan_InitInfo initInfo = {};


	CheckReturn(mpLogFile, ImGui_ImplVulkan_Init(&initInfo));

	mbIsVulkanInitialized = TRUE;

	return TRUE;
}

void VkImGuiManager::CleanUpVulkan() {
	if (mbIsVulkanInitialized) ImGui_ImplVulkan_Shutdown();
}