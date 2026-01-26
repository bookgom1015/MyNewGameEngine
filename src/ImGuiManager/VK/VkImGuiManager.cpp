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

BOOL VkImGuiManager::InitializeVulkan(
		const VkInstance& instance,
		const VkPhysicalDevice& physicalDevice,
		const VkDevice& device,
		const VkQueue& graphicsQueue,
		const VkDescriptorPool& descriptorPool,
		VkFormat swapchainImageFormat,
		std::uint32_t minImageCount,
		std::uint32_t imageCount) {
	ImGui::SetCurrentContext(mpContext);

	ImGui_ImplVulkan_InitInfo initInfo{};
	initInfo.Instance = instance;
	initInfo.PhysicalDevice = physicalDevice;
	initInfo.Device = device;
	initInfo.Queue = graphicsQueue;
	initInfo.DescriptorPool = descriptorPool;
	initInfo.MinImageCount = minImageCount;
	initInfo.ImageCount = imageCount;
	initInfo.UseDynamicRendering = true;
	initInfo.PipelineRenderingCreateInfo = { 
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
	initInfo.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapchainImageFormat;
	initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	CheckReturn(mpLogFile, ImGui_ImplVulkan_Init(&initInfo));
	//CheckReturn(mpLogFile, ImGui_ImplVulkan_CreateFontsTexture());


	mbIsVulkanInitialized = TRUE;

	return TRUE;
}

void VkImGuiManager::CleanUp() {
	if (mbIsVulkanInitialized) ImGui_ImplVulkan_Shutdown();
	ImGuiManager::CleanUp();
}