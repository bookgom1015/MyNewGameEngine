#pragma once

#include <optional>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <Windows.h>

#ifndef VK_USE_PLATFORM_WIN32_KHR
	#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

namespace Common::Debug {
	struct LogFile;
}

namespace Render::VK::Foundation::Util {
	struct QueueFamilyIndices {
		std::optional<UINT> GraphicsFamily;
		std::optional<UINT> PresentFamily;

		UINT GetGraphicsFamilyIndex();
		UINT GetPresentFamilyIndex();
		BOOL IsComplete();

		static QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface);
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR Capabilities;
		std::vector<VkSurfaceFormatKHR> Formats;
		std::vector<VkPresentModeKHR> PresentModes;
	};

	class VulkanUtil {
	public:
		static const std::vector<const CHAR*> ValidationLayers;
		static const std::vector<const CHAR*> DeviceExtensions;
		static const std::vector<const CHAR*> RequiredExtensions;

	public:
		static BOOL CheckValidationLayersSupport(Common::Debug::LogFile* const pLogFile);
		static VkResult CreateDebugUtilsMessengerEXT(
			const VkInstance& instance,
			VkDebugUtilsMessengerCreateInfoEXT* const pCreateInfo,
			VkAllocationCallbacks* const pAllocator,
			VkDebugUtilsMessengerEXT* const pDebugMessenger);
		static void DestroyDebugUtilsMessengerEXT(
			const VkInstance& instance,
			VkDebugUtilsMessengerEXT debugMessenger,
			VkAllocationCallbacks* const pAllocator);
		static void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo, void* pUserData);
		static BOOL SetUpDebugMessenger(Common::Debug::LogFile* const pLogFile, const VkInstance& instance, VkDebugUtilsMessengerEXT& debugMessenger, void* pUserData);

		static BOOL CheckDeviceExtensionsSupport(const VkPhysicalDevice& physicalDevice);
		static SwapChainSupportDetails QuerySwapChainSupport(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface);
		static BOOL IsDeviceSuitable(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface);
		static INT RateDeviceSuitability(Common::Debug::LogFile* const pLogFile, const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface);
		static BOOL ShowDeviceInfo(Common::Debug::LogFile* const pLogFile, const VkPhysicalDevice& physicalDevice);
		static BOOL IsRaytracingSupported(VkPhysicalDevice physicalDevice);

		static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& inAvailableFormats);
		static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& inAvailablePresentMods);
		static VkExtent2D ChooseSwapExtent(UINT width, UINT height, const VkSurfaceCapabilitiesKHR& inCapabilities);

		static BOOL BeginSingleTimeCommands(
			Common::Debug::LogFile* const pLogFile, 
			const VkDevice& device, 
			const VkCommandPool& commandPool, 
			VkCommandBuffer& commandBuffer);
		static BOOL EndSingleTimeCommands(
			Common::Debug::LogFile* const pLogFile, 
			const VkDevice& device, 
			const VkQueue& queue, 
			const VkCommandPool& commandPool, 
			VkCommandBuffer& commandBuffer);

		static UINT FindMemoryType(const VkPhysicalDevice& physicalDevice, UINT typeFilter, const VkMemoryPropertyFlags& properties);

		static BOOL CreateBuffer(
			Common::Debug::LogFile* const pLogFile,
			const VkPhysicalDevice& physicalDevice,
			const VkDevice& device,
			VkDeviceSize size,
			const VkBufferUsageFlags& usage,
			const VkMemoryPropertyFlags& properties,
			VkBuffer& buffer,
			VkDeviceMemory& bufferMemory);
		static BOOL CopyBuffer(
			Common::Debug::LogFile* const pLogFile,
			const VkDevice& device,
			const VkQueue& queue,
			const VkCommandPool& commandPool,
			const VkBuffer& srcBuffer,
			const VkBuffer& dstBuffer,
			VkDeviceSize size);
		static BOOL CopyBufferToImage(
			Common::Debug::LogFile* const pLogFile,
			const VkDevice& device,
			const VkQueue& queue,
			const VkCommandPool& commandPool,
			const VkBuffer& buffer,
			const VkImage& image,
			UINT width,
			UINT height);
		static BOOL CreateImage(
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
			VkDeviceMemory& imageMemory);
		static BOOL CreateImageView(
			Common::Debug::LogFile* const pLogFile,
			const VkDevice& device,
			const VkImage& image,
			const VkFormat& format,
			UINT mipLevles,
			const VkImageAspectFlags& aspectFlags,
			VkImageView& imageView);
	};
}