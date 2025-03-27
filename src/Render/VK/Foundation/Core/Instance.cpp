#include "Render/VK/Foundation/Core/Instance.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/VK/Foundation/Util/VulkanUtil.hpp"

using namespace Render::VK::Foundation::Core;

Instance::~Instance() {
	CleanUp();
}
	
BOOL Instance::Initalize(Common::Debug::LogFile* const pLogFile) {
	mpLogFile = pLogFile;

	CheckReturn(mpLogFile, CreateInstance());
#ifdef _DEBUG
	CheckReturn(mpLogFile, Foundation::Util::VulkanUtil::SetUpDebugMessenger(mpLogFile, mInstance, mDebugMessenger, mpLogFile));
#endif

	return TRUE;
}

void Instance::CleanUp() {
#ifdef _DEBUG
	Foundation::Util::VulkanUtil::DestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);
#endif
	vkDestroyInstance(mInstance, nullptr);
}

BOOL Instance::CreateInstance() {
#ifdef _DEBUG
	CheckReturn(mpLogFile, Foundation::Util::VulkanUtil::CheckValidationLayersSupport(mpLogFile));
#endif

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "MyNewGameEngine";
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
	appInfo.pEngineName = "New Game Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	UINT availableExtensionCount = 0;
	if (vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, nullptr) != VK_SUCCESS)
		ReturnFalse(mpLogFile, L"Failed to get number of instance extension properties");

	std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
	if (vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, availableExtensions.data()) != VK_SUCCESS)
		ReturnFalse(mpLogFile, L"Failed to get instance extension properties");

	std::vector<const CHAR*> missingExtensions;
	BOOL status = TRUE;


	for (const auto& requiredExt : Foundation::Util::VulkanUtil::RequiredExtensions) {
		BOOL supported = FALSE;

		for (const auto& availableExt : availableExtensions) {
			if (std::strcmp(requiredExt, availableExt.extensionName) == 0) {
				supported = TRUE;
				break;
			}
		}

		if (!supported) {
			missingExtensions.push_back(requiredExt);
			status = FALSE;
		}
	}

	if (!status) {
		WLogln(mpLogFile, L"Upsupported extensions:");
		for (const auto& missingExt : missingExtensions)
			Logln(mpLogFile, "    ", missingExt);

		return FALSE;
	}

	createInfo.enabledExtensionCount = static_cast<UINT>(Foundation::Util::VulkanUtil::RequiredExtensions.size());
	createInfo.ppEnabledExtensionNames = Foundation::Util::VulkanUtil::RequiredExtensions.data();

#ifdef _DEBUG
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;

	createInfo.enabledLayerCount = static_cast<UINT>(Foundation::Util::VulkanUtil::ValidationLayers.size());
	createInfo.ppEnabledLayerNames = Foundation::Util::VulkanUtil::ValidationLayers.data();

	Foundation::Util::VulkanUtil::PopulateDebugMessengerCreateInfo(debugCreateInfo, mpLogFile);
	createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
#else
	createInfo.enabledLayerCount = 0;
	createInfo.pNext = nullptr;
#endif

	if (vkCreateInstance(&createInfo, nullptr, &mInstance) != VK_SUCCESS)
		ReturnFalse(mpLogFile, L"Failed to create instance");

	return TRUE;
}