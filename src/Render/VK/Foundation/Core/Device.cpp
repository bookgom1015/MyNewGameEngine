#include "Render/VK/Foundation/Core/Device.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Util/StringUtil.hpp"
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

	return TRUE;
}

void Device::CleanUp() {
	if (mDevice != VK_NULL_HANDLE)
		vkDestroyDevice(mDevice, nullptr);
}

BOOL Device::SortPhysicalDevices() {
	UINT deviceCount = 0;
	vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr);
	if (deviceCount == 0) ReturnFalse(mpLogFile, L"Failed to find GPU(s) with Vulkan support");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(mInstance, &deviceCount, devices.data());

	for (const auto& device : devices) {
		INT score = Foundation::Util::VulkanUtil::RateDeviceSuitability(mpLogFile, device, mSurface);
		mCandidates.emplace_back(score, device);
	}

	std::sort(mCandidates.begin(), mCandidates.end(), [](const auto& a, const auto& b) {
		return a.first > b.first;
	});

	for (const auto& pair : mCandidates) {
		const auto device = pair.second;
		CheckReturn(mpLogFile, Foundation::Util::VulkanUtil::ShowDeviceInfo(mpLogFile, device));
	}

	return TRUE;
}

BOOL Device::GetPhysicalDevices(std::vector<std::wstring>& physicalDevices) {
	for (auto begin = mCandidates.begin(), end = mCandidates.end(); begin != end; ++begin) {
		const auto& physicalDevice = begin->second;

		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

		physicalDevices.push_back(Common::Util::StringUtil::StringToWString(deviceProperties.deviceName));
	}

	return TRUE;
}

BOOL Device::SelectPhysicalDevices(UINT selectedPhysicalDeviceIndex, BOOL& bRaytracingSupported) {
	mPhysicalDevice = mCandidates[selectedPhysicalDeviceIndex].second;

	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(mPhysicalDevice, &deviceProperties);
	Logln(mpLogFile, "Selected physical device: ", deviceProperties.deviceName);

	if (Foundation::Util::VulkanUtil::IsRaytracingSupported(mPhysicalDevice)) {
		bRaytracingSupported = TRUE;
		Logln(mpLogFile, deviceProperties.deviceName, " supports ray-tracing");
	}
	else {
		bRaytracingSupported = FALSE;
		Logln(mpLogFile, deviceProperties.deviceName, " does not support ray-tracing");
	}

	CheckReturn(mpLogFile, CreateLogicalDevice());

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