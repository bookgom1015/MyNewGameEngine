#include "Render/VK/VkLowRenderer.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/VK/Foundation/VulkanUtil.hpp"

#include <algorithm>
#include <set>

using namespace Render::VK;

VkLowRenderer::VkLowRenderer() : mpLogFile(nullptr) {
	mInstance = {};
}

VkLowRenderer::~VkLowRenderer() {

}

BOOL VkLowRenderer::Initialize(Common::Debug::LogFile* const pLogFile, HWND hWnd, UINT width, UINT height) {
	mhMainWnd = hWnd;
	mpLogFile = pLogFile;

	mClientWidth = width;
	mClientHeight = height;

	CheckReturn(mpLogFile, CreateInstance());	
#ifdef _DEBUG
	CheckReturn(mpLogFile, Foundation::VulkanUtil::SetUpDebugMessenger(mpLogFile, mInstance, mDebugMessenger, mpLogFile));
#endif
	CheckReturn(mpLogFile, CreateSurface());
	CheckReturn(mpLogFile, SelectPhysicalDevice());
	CheckReturn(mpLogFile, CreateLogicalDevice());
	CheckReturn(mpLogFile, CreateSwapChain());
	CheckReturn(mpLogFile, CreateImageViews());

	return TRUE;
}

void VkLowRenderer::CleanUp() {
	for (size_t i = 0; i < SwapChainImageCount; ++i) 
		vkDestroyImageView(mDevice, mSwapChainImageViews[i], nullptr);
	vkDestroySwapchainKHR(mDevice, mSwapChain, nullptr);
	vkDestroyDevice(mDevice, nullptr);
	vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
#ifdef _DEBUG
	Foundation::VulkanUtil::DestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);
#endif
	vkDestroyInstance(mInstance, nullptr);
}

BOOL VkLowRenderer::OnResize(UINT width, UINT height) {
	CheckReturn(mpLogFile, CreateSwapChain());
	CheckReturn(mpLogFile, CreateImageViews());

	return TRUE;
}

BOOL VkLowRenderer::CreateInstance() {
#ifdef _DEBUG
	CheckReturn(mpLogFile, Foundation::VulkanUtil::CheckValidationLayersSupport(mpLogFile));
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


	for (const auto& requiredExt : Foundation::VulkanUtil::RequiredExtensions) {
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

	createInfo.enabledExtensionCount = static_cast<UINT>(Foundation::VulkanUtil::RequiredExtensions.size());
	createInfo.ppEnabledExtensionNames = Foundation::VulkanUtil::RequiredExtensions.data();

#ifdef _DEBUG
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;

	createInfo.enabledLayerCount = static_cast<UINT>(Foundation::VulkanUtil::ValidationLayers.size());
	createInfo.ppEnabledLayerNames = Foundation::VulkanUtil::ValidationLayers.data();

	Foundation::VulkanUtil::PopulateDebugMessengerCreateInfo(debugCreateInfo, mpLogFile);
	createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
#else
	createInfo.enabledLayerCount = 0;
	createInfo.pNext = nullptr;
#endif

	if (vkCreateInstance(&createInfo, nullptr, &mInstance) != VK_SUCCESS) 
		ReturnFalse(mpLogFile, L"Failed to create instance");

	return TRUE;
}

BOOL VkLowRenderer::CreateSurface() {
	VkWin32SurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hwnd = mhMainWnd;
	createInfo.hinstance = GetModuleHandle(nullptr);

	if (vkCreateWin32SurfaceKHR(mInstance, &createInfo, nullptr, &mSurface) != VK_SUCCESS)
		ReturnFalse(mpLogFile, L"Failed to create surface");
	
	return TRUE;
}

BOOL VkLowRenderer::SelectPhysicalDevice() {
	UINT deviceCount = 0;
	vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr);
	if (deviceCount == 0) ReturnFalse(mpLogFile, L"Failed to find GPU(s) with Vulkan support");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(mInstance, &deviceCount, devices.data());

	std::vector<std::pair<INT, VkPhysicalDevice>> candidates;
	for (const auto& device : devices) {
		INT score = Foundation::VulkanUtil::RateDeviceSuitability(mpLogFile, device, mSurface);
		candidates.emplace_back(score, device);
	}

	std::sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) {
		return a.first > b.first;
	});

	BOOL found = FALSE;
	for (auto begin = candidates.begin(), end = candidates.end(); begin != end; ++begin) {
		const auto& physicalDevice = begin->second;
		mPhysicalDevice = physicalDevice;

		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
		Logln(mpLogFile, "Selected physical device: ", deviceProperties.deviceName);

		found = TRUE;
		break;
	}
	if (!found) ReturnFalse(mpLogFile, L"Failed to find a suitable GPU");

	return TRUE;
}

BOOL VkLowRenderer::CreateLogicalDevice() {
	Foundation::QueueFamilyIndices indices = Foundation::QueueFamilyIndices::FindQueueFamilies(mPhysicalDevice, mSurface);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<UINT> uniqueQueueFamilies = {
		indices.GetGraphicsFamilyIndex(),
		indices.GetPresentFamilyIndex()
	};

	FLOAT queuePriority = 1.f;
	for (UINT queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.sampleRateShading = VK_TRUE;

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = static_cast<UINT>(queueCreateInfos.size());
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<UINT>(Foundation::VulkanUtil::DeviceExtensions.size());
	createInfo.ppEnabledExtensionNames = Foundation::VulkanUtil::DeviceExtensions.data();
#ifdef _DEBUG
	createInfo.enabledLayerCount = static_cast<UINT>(Foundation::VulkanUtil::ValidationLayers.size());
	createInfo.ppEnabledLayerNames = Foundation::VulkanUtil::ValidationLayers.data();
#else
	createInfo.enabledLayerCount = 0;
#endif

	if (vkCreateDevice(mPhysicalDevice, &createInfo, nullptr, &mDevice) != VK_SUCCESS)
		ReturnFalse(mpLogFile, L"Failed to create logical device");

	vkGetDeviceQueue(mDevice, indices.GetGraphicsFamilyIndex(), 0, &mGraphicsQueue);
	vkGetDeviceQueue(mDevice, indices.GetPresentFamilyIndex(), 0, &mPresentQueue);

	return TRUE;
}

BOOL VkLowRenderer::CreateSwapChain() {
	Foundation::SwapChainSupportDetails swapChainSupport = Foundation::VulkanUtil::QuerySwapChainSupport(mPhysicalDevice, mSurface);

	VkSurfaceFormatKHR surfaceFormat = Foundation::VulkanUtil::ChooseSwapSurfaceFormat(swapChainSupport.Formats);
	VkPresentModeKHR presentMode = Foundation::VulkanUtil::ChooseSwapPresentMode(swapChainSupport.PresentModes);
	VkExtent2D extent = Foundation::VulkanUtil::ChooseSwapExtent(mClientWidth, mClientHeight, swapChainSupport.Capabilities);

	UINT imageCount = SwapChainImageCount;

	if (imageCount < swapChainSupport.Capabilities.minImageCount || 
			(swapChainSupport.Capabilities.maxImageCount > 0 && imageCount > swapChainSupport.Capabilities.maxImageCount)) {
		ReturnFalse(mpLogFile, L"Swap-chain does not support the specified number of images");
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = mSurface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	Foundation::QueueFamilyIndices indices = Foundation::QueueFamilyIndices::FindQueueFamilies(mPhysicalDevice, mSurface);
	UINT queueFamilyIndices[] = {
		indices.GetGraphicsFamilyIndex(),
		indices.GetPresentFamilyIndex()
	};

	if (indices.GraphicsFamily != indices.PresentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 1;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	createInfo.preTransform = swapChainSupport.Capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(mDevice, &createInfo, nullptr, &mSwapChain) != VK_SUCCESS) 
		ReturnFalse(mpLogFile, L"Failed to create swap chain");

	if (vkGetSwapchainImagesKHR(mDevice, mSwapChain, &imageCount, mSwapChainImages.data()) != VK_SUCCESS)
		ReturnFalse(mpLogFile, L"Failed to get swap chain images");

	mSwapChainImageFormat = surfaceFormat.format;
	mSwapChainExtent = extent;

	return TRUE;
}

BOOL VkLowRenderer::CreateImageViews() {
	for (size_t i = 0, end = mSwapChainImages.size(); i < end; ++i) {
		CheckReturn(mpLogFile, Foundation::VulkanUtil::CreateImageView(
			mpLogFile,
			mDevice, 
			mSwapChainImages[i], 
			mSwapChainImageFormat, 
			1, 
			VK_IMAGE_ASPECT_COLOR_BIT, 
			mSwapChainImageViews[i]
		));
	}

	return TRUE;
}
