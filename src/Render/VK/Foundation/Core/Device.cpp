#include "Render/VK/Foundation/Core/Device.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/VK/Foundation/Util/VulkanUtil.hpp"

#include <algorithm>
#include <set>

using namespace Render::VK::Foundation::Core;

Device::~Device() {
	CleanUp();
}

BOOL Device::Initialize(Common::Debug::LogFile* const pLogFile, VkInstance instance, VkSurfaceKHR surface) {
	mpLogFile = pLogFile;
	mInstance = instance;
	mSurface = surface;

	CheckReturn(mpLogFile, SelectPhysicalDevice());
	CheckReturn(mpLogFile, CreateLogicalDevice());

	return TRUE;
}

void Device::CleanUp() {
	vkDestroyDevice(mDevice, nullptr);
}

BOOL Device::SelectPhysicalDevice() {
	UINT deviceCount = 0;
	vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr);
	if (deviceCount == 0) ReturnFalse(mpLogFile, L"Failed to find GPU(s) with Vulkan support");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(mInstance, &deviceCount, devices.data());

	std::vector<std::pair<INT, VkPhysicalDevice>> candidates;
	for (const auto& device : devices) {
		INT score = Foundation::Util::VulkanUtil::RateDeviceSuitability(mpLogFile, device, mSurface);
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

BOOL Device::CreateLogicalDevice() {
	Foundation::Util::QueueFamilyIndices indices = Foundation::Util::QueueFamilyIndices::FindQueueFamilies(mPhysicalDevice, mSurface);

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
	createInfo.enabledExtensionCount = static_cast<UINT>(Foundation::Util::VulkanUtil::DeviceExtensions.size());
	createInfo.ppEnabledExtensionNames = Foundation::Util::VulkanUtil::DeviceExtensions.data();
#ifdef _DEBUG
	createInfo.enabledLayerCount = static_cast<UINT>(Foundation::Util::VulkanUtil::ValidationLayers.size());
	createInfo.ppEnabledLayerNames = Foundation::Util::VulkanUtil::ValidationLayers.data();
#else
	createInfo.enabledLayerCount = 0;
#endif

	if (vkCreateDevice(mPhysicalDevice, &createInfo, nullptr, &mDevice) != VK_SUCCESS)
		ReturnFalse(mpLogFile, L"Failed to create logical device");

	return TRUE;
}