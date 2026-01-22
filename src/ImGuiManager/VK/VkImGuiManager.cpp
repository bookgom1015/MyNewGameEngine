#include "ImGuiManager/VK/pch_imgui_vk.h"
#include "ImGuiManager/VK/VkImGuiManager.hpp"
#include "Common/Debug/Logger.hpp"

using namespace ImGuiManager::VK;

extern "C" ImGuiManagerAPI Common::ImGuiManager::ImGuiManager* ImGuiManager::CreateImGuiManager() {
	return new VkImGuiManager();
}

extern "C" ImGuiManagerAPI void ImGuiManager::DestroyImGuiManager(Common::ImGuiManager::ImGuiManager* const imGuiManager) {
	delete imGuiManager;
}

VkImGuiManager::VkImGuiManager() {}

VkImGuiManager::~VkImGuiManager() {}

BOOL VkImGuiManager::InitializeVulkan(const VkInstance& instance) {
	ImGui::SetCurrentContext(mpContext);

	ImGui_ImplVulkan_InitInfo initInfo{};
	initInfo.Instance = instance;
	CheckReturn(mpLogFile, ImGui_ImplVulkan_Init(&initInfo));

	mbIsVulkanInitialized = TRUE;

	return TRUE;
}

void VkImGuiManager::CleanUp() {
	if (mbIsVulkanInitialized) ImGui_ImplVulkan_Shutdown();
	ImGuiManager::CleanUp();
}