#pragma once

#include <string>
#include <vector>

#ifndef VK_USE_PLATFORM_WIN32_KHR
	#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

namespace Common::Debug {
	struct LogFile;
}

namespace Render::VK::Foundation::Core {
	class Device {
	public:
		Device() = default;
		virtual ~Device();

	public:
		BOOL Initialize(Common::Debug::LogFile* const pLogFile, VkInstance instance, VkSurfaceKHR surface);
		void CleanUp();

	public:
		__forceinline VkPhysicalDevice PhysicalDevice() const;
		__forceinline VkDevice LogicalDevice() const;

	public:
		BOOL SortPhysicalDevices();
		BOOL GetPhysicalDevices(std::vector<std::wstring>& physicalDevices);
		BOOL SelectPhysicalDevices(UINT selectedPhysicalDeviceIndex, BOOL& bRaytracingSupported);

	private:
		BOOL CreateLogicalDevice();

	private:
		Common::Debug::LogFile* mpLogFile;

		VkInstance mInstance;
		VkSurfaceKHR mSurface;

		std::vector<std::pair<INT, VkPhysicalDevice>> mCandidates;

		VkPhysicalDevice mPhysicalDevice;
		VkDevice mDevice;
	};
}

#include "Render/VK/Foundation/Core/Device.inl"