#include "Render/VK/Foundation/Util/VulkanUtil.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Mesh/Vertex.h"

#include <set>

#undef min
#undef max

using namespace Render::VK::Foundation::Util;

namespace {
	VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData) {
		const auto logFile = reinterpret_cast<Common::Debug::LogFile*>(pUserData);
		Logln(logFile, pCallbackData->pMessage);

		return VK_FALSE;
	}
}

const std::vector<const CHAR*> VulkanUtil::ValidationLayers = { "VK_LAYER_KHRONOS_validation" };
const std::vector<const CHAR*> VulkanUtil::DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
const std::vector<const CHAR*> VulkanUtil::RequiredExtensions = {  
	VK_KHR_SURFACE_EXTENSION_NAME, 
	VK_KHR_WIN32_SURFACE_EXTENSION_NAME 
#ifdef _DEBUG
	, VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif
};

UINT QueueFamilyIndices::GetGraphicsFamilyIndex() { return GraphicsFamily.value(); }

UINT QueueFamilyIndices::GetPresentFamilyIndex() { return PresentFamily.value(); }

BOOL QueueFamilyIndices::IsComplete() { return GraphicsFamily.has_value() && PresentFamily.has_value(); }

QueueFamilyIndices QueueFamilyIndices::FindQueueFamilies(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface) {
	QueueFamilyIndices indices = {};

	UINT queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	UINT i = 0;
	for (const auto& queueFamiliy : queueFamilies) {
		if (queueFamiliy.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.GraphicsFamily = i;
		}

		VkBool32 presentSupport = FALSE;
		if (vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport) != VK_SUCCESS) continue;

		if (presentSupport) indices.PresentFamily = i;
		if (indices.IsComplete()) break;

		++i;
	}

	return indices;
}

BOOL VulkanUtil::CheckValidationLayersSupport(Common::Debug::LogFile* const pLogFile) {
	UINT layerCount = 0;
	if (vkEnumerateInstanceLayerProperties(&layerCount, nullptr) != VK_SUCCESS) 
		ReturnFalse(pLogFile, L"Failed to get numbuer of instance layer properties");

	std::vector<VkLayerProperties> availableLayers(layerCount);
	if (vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()) != VK_SUCCESS) 
		ReturnFalse(pLogFile, L"Failed to get instance layer properties");

#ifdef _DEBUG
	WLogln(pLogFile, L"Available Layers:");
	for (const auto& layer : availableLayers) 
		Logln(pLogFile, "    ", layer.layerName);
#endif

	std::vector<const CHAR*> unsupportedLayers;
	BOOL status = TRUE;

	for (auto layerName : ValidationLayers) {
		BOOL layerFound = FALSE;

		for (const auto& layer : availableLayers) {
			if (std::strcmp(layerName, layer.layerName) == 0) {
				layerFound = TRUE;
				break;
			}
		}

		if (!layerFound) {
			unsupportedLayers.push_back(layerName);
			status = FALSE;
		}
	}

	if (!status) {
		std::wstringstream wsstream;

		for (const auto& layerName : unsupportedLayers)
			wsstream << layerName << L' ';
		wsstream << L"is/are not supported";

		ReturnFalse(pLogFile, wsstream.str());
	}

	return TRUE;
}

VkResult VulkanUtil::CreateDebugUtilsMessengerEXT(
		const VkInstance& instance,
		VkDebugUtilsMessengerCreateInfoEXT* const pCreateInfo,
		VkAllocationCallbacks* const pAllocator,
		VkDebugUtilsMessengerEXT* const pDebugMessenger) {
	const auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	else return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void VulkanUtil::DestroyDebugUtilsMessengerEXT(
		const VkInstance& instance,
		VkDebugUtilsMessengerEXT debugMessenger,
		VkAllocationCallbacks* const pAllocator) {
	const auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) func(instance, debugMessenger, pAllocator);
}

void VulkanUtil::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo, void* pUserData) {
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = DebugCallback;
	createInfo.pUserData = pUserData;
}

BOOL VulkanUtil::SetUpDebugMessenger(Common::Debug::LogFile* const pLogFile, const VkInstance& instance, VkDebugUtilsMessengerEXT& debugMessenger, void* pUserData) {
	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	PopulateDebugMessengerCreateInfo(createInfo, pUserData);

	if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) 
		ReturnFalse(pLogFile, L"Failed to create debug messenger");

	return TRUE;
}

BOOL VulkanUtil::CheckDeviceExtensionsSupport(const VkPhysicalDevice& physicalDevice) {
	UINT extensionCount = 0;
	if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr) != VK_SUCCESS)
		return FALSE;

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data()) != VK_SUCCESS)
		return FALSE;

	std::set<std::string> requiredExtensions(DeviceExtensions.begin(), DeviceExtensions.end());
	for (const auto& extension : availableExtensions) 
		requiredExtensions.erase(extension.extensionName);

	return requiredExtensions.empty();
}

SwapChainSupportDetails VulkanUtil::QuerySwapChainSupport(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface) {
	SwapChainSupportDetails details = {};

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.Capabilities);

	UINT formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
	if (formatCount != 0) {
		details.Formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.Formats.data());
	}

	UINT presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
	if (presentModeCount != 0) {
		details.PresentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.PresentModes.data());
	}

	return details;
}

BOOL VulkanUtil::IsDeviceSuitable(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface) {
	QueueFamilyIndices indices = QueueFamilyIndices::FindQueueFamilies(physicalDevice, surface);

	BOOL extensionSupported = CheckDeviceExtensionsSupport(physicalDevice);
	BOOL swapChainAdequate = FALSE;
	if (extensionSupported) {
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(physicalDevice, surface);
		swapChainAdequate = !swapChainSupport.Formats.empty() && !swapChainSupport.PresentModes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

	return indices.IsComplete() && extensionSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

INT VulkanUtil::RateDeviceSuitability(Common::Debug::LogFile* const pLogFile, const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface) {
	INT score = 0;

	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

	switch (deviceProperties.deviceType) {
	case VK_PHYSICAL_DEVICE_TYPE_OTHER:
		score += 100;
		break;
	case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
		score += 300;
		break;
	case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
		score += 1000;
		break;
	case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
		score += 150;
		break;
	case VK_PHYSICAL_DEVICE_TYPE_CPU:
		score += 10;
		break;
	}
	score += deviceProperties.limits.maxImageDimension2D;

	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

	if (!deviceFeatures.geometryShader || !IsDeviceSuitable(physicalDevice, surface)) {
		WLogln(pLogFile, L"    This device is not suitable");
		return 0;
	}

	return score;
}

BOOL VulkanUtil::ShowDeviceInfo(Common::Debug::LogFile* const pLogFile, const VkPhysicalDevice& physicalDevice) {
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

	WLogln(pLogFile, L"Physical device properties:");
	Logln(pLogFile, "    Device name: ", deviceProperties.deviceName);

	std::wstringstream wsstream;
	wsstream << L"    Device type: ";
	switch (deviceProperties.deviceType) {
	case VK_PHYSICAL_DEVICE_TYPE_OTHER:
		wsstream << L"Other type";
		break;
	case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
		wsstream << L"Integrated GPU";
		break;
	case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
		wsstream << L"Discrete GPU";
		break;
	case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
		wsstream << L"Virtual GPU";
		break;
	case VK_PHYSICAL_DEVICE_TYPE_CPU:
		wsstream << L"CPU type";
		break;
	}
	WLogln(pLogFile, wsstream.str().c_str());

	{
		UINT variant = VK_API_VERSION_VARIANT(deviceProperties.driverVersion);
		UINT major = VK_API_VERSION_MAJOR(deviceProperties.driverVersion);
		UINT minor = VK_API_VERSION_MINOR(deviceProperties.driverVersion);
		UINT patch = VK_API_VERSION_PATCH(deviceProperties.driverVersion);
		WLogln(pLogFile, L"    Device version: ",
			std::to_wstring(variant), L".",
			std::to_wstring(major), L".",
			std::to_wstring(minor), L".",
			std::to_wstring(patch));
	}
	{
		UINT variant = VK_API_VERSION_VARIANT(deviceProperties.apiVersion);
		UINT major = VK_API_VERSION_MAJOR(deviceProperties.apiVersion);
		UINT minor = VK_API_VERSION_MINOR(deviceProperties.apiVersion);
		UINT patch = VK_API_VERSION_PATCH(deviceProperties.apiVersion);
		WLogln(pLogFile, L"    API version: ",
			std::to_wstring(variant), L".",
			std::to_wstring(major), L".",
			std::to_wstring(minor), L".",
			std::to_wstring(patch));
	}

	return TRUE;
}

BOOL VulkanUtil::IsRaytracingSupported(VkPhysicalDevice physicalDevice) {
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensions.data());

	for (const auto& ext : extensions) {
		if (strcmp(ext.extensionName, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME) == 0) 
			return TRUE;
	}

	return FALSE;
}

VkSurfaceFormatKHR VulkanUtil::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& inAvailableFormats) {
	for (const auto& availableFormat : inAvailableFormats) {
		if (availableFormat.format == VK_FORMAT_R8G8B8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}
	return inAvailableFormats[0];
}

VkPresentModeKHR VulkanUtil::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& inAvailablePresentMods) {
	for (const auto& availablePresentMode : inAvailablePresentMods) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanUtil::ChooseSwapExtent(UINT width, UINT height, const VkSurfaceCapabilitiesKHR& inCapabilities) {
	if (inCapabilities.currentExtent.width != UINT32_MAX) {
		return inCapabilities.currentExtent;
	}

	VkExtent2D actualExtent = { static_cast<UINT>(width), static_cast<UINT>(height) };

	actualExtent.width = std::max(inCapabilities.minImageExtent.width, std::min(inCapabilities.maxImageExtent.width, actualExtent.width));
	actualExtent.height = std::max(inCapabilities.minImageExtent.height, std::min(inCapabilities.maxImageExtent.height, actualExtent.height));

	return actualExtent;
}

BOOL VulkanUtil::BeginSingleTimeCommands(
		Common::Debug::LogFile* const pLogFile, 
		const VkDevice& device, 
		const VkCommandPool& commandPool, 
		VkCommandBuffer& commandBuffer) {
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS)
		ReturnFalse(pLogFile, L"Failed to allocate command buffers");

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
		ReturnFalse(pLogFile, L"Failed to begin command buffer");

	return TRUE;
}

BOOL VulkanUtil::EndSingleTimeCommands(
		Common::Debug::LogFile* const pLogFile, 
		const VkDevice& device, 
		const VkQueue& queue, 
		const VkCommandPool& commandPool, 
		VkCommandBuffer& commandBuffer) {
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	if (vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) ReturnFalse(pLogFile, L"Failed to submit queue");
	if (vkQueueWaitIdle(queue) != VK_SUCCESS) ReturnFalse(pLogFile, L"Failed to wait queue");

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);

	return TRUE;
}

UINT VulkanUtil::FindMemoryType(const VkPhysicalDevice& physicalDevice, UINT typeFilter, const VkMemoryPropertyFlags& properties) {
	UINT index = std::numeric_limits<UINT>::max();

	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (UINT i = 0; i < memProperties.memoryTypeCount; ++i) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			index = i;
			break;
		}
	}

	return index;
}

BOOL VulkanUtil::CreateBuffer(
		Common::Debug::LogFile* const pLogFile,
		const VkPhysicalDevice& physicalDevice,
		const VkDevice& device,
		VkDeviceSize size,
		const VkBufferUsageFlags& usage,
		const VkMemoryPropertyFlags& properties,
		VkBuffer& buffer,
		VkDeviceMemory& bufferMemory) {
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.flags = 0;

	if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) 
		ReturnFalse(pLogFile, L"Failed to create vertex buffer");

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) 
		ReturnFalse(pLogFile, L"Failed to allocate vertex buffer memory");

	if (vkBindBufferMemory(device, buffer, bufferMemory, 0) != VK_SUCCESS)
		ReturnFalse(pLogFile, L"Failed to bind buffer memory");

	return TRUE;
}

BOOL VulkanUtil::CopyBuffer(
		Common::Debug::LogFile* const pLogFile,
		const VkDevice& device,
		const VkQueue& queue,
		const VkCommandPool& commandPool,
		const VkBuffer& srcBuffer,
		const VkBuffer& dstBuffer,
		VkDeviceSize size) {
	VkCommandBuffer commandBuffer; 
	CheckReturn(pLogFile, BeginSingleTimeCommands(pLogFile, device, commandPool, commandBuffer));

	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	CheckReturn(pLogFile, EndSingleTimeCommands(pLogFile, device, queue, commandPool, commandBuffer));

	return TRUE;
}

BOOL VulkanUtil::CopyBufferToImage(
		Common::Debug::LogFile* const pLogFile,
		const VkDevice& device,
		const VkQueue& queue,
		const VkCommandPool& commandPool,
		const VkBuffer& buffer,
		const VkImage& image,
		UINT width,
		UINT height) {
	VkCommandBuffer commandBuffer; 
	CheckReturn(pLogFile, BeginSingleTimeCommands(pLogFile, device, commandPool, commandBuffer));

	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width, height, 1 };

	vkCmdCopyBufferToImage(
		commandBuffer,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region);

	CheckReturn(pLogFile, EndSingleTimeCommands(pLogFile, device, queue, commandPool, commandBuffer));

	return TRUE;
}

BOOL VulkanUtil::CreateImage(
		Common::Debug::LogFile* const pLogFile,
		const VkPhysicalDevice& physicalDevice,
		const VkDevice& device,
		UINT width,
		UINT height,
		UINT mipLevels,
		const VkSampleCountFlagBits& numSamples,
		const VkFormat& format,
		const VkImageTiling& tiling,
		const VkImageUsageFlags& usage,
		const VkMemoryPropertyFlags& properties,
		VkImage& image,
		VkDeviceMemory& imageMemory) {
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = static_cast<UINT>(width);
	imageInfo.extent.height = static_cast<UINT>(height);
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = numSamples;
	imageInfo.flags = 0;

	if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) 
		ReturnFalse(pLogFile, L"Failed to create image");

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) 
		ReturnFalse(pLogFile, L"Failed to allocate texture image memory");

	if (vkBindImageMemory(device, image, imageMemory, 0) != VK_SUCCESS)
		ReturnFalse(pLogFile, L"Failed to bind memory to image");

	return TRUE;
}

BOOL VulkanUtil::CreateImageView(
		Common::Debug::LogFile* const pLogFile,
		const VkDevice& device,
		const VkImage& image,
		const VkFormat& format,
		UINT mipLevles,
		const VkImageAspectFlags& aspectFlags,
		VkImageView& imageView) {
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipLevles;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) 
		ReturnFalse(pLogFile, L"Failed to create texture image view");

	return TRUE;
}

VkVertexInputBindingDescription VulkanUtil::GetVertexBindingDescription() {
	VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(Common::Foundation::Mesh::Vertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 3> VulkanUtil::GetVertexAttributeDescriptions() {
	std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Common::Foundation::Mesh::Vertex, Position);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Common::Foundation::Mesh::Vertex, Normal);

	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Common::Foundation::Mesh::Vertex, TexCoord);

	return attributeDescriptions;
}