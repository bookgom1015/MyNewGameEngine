#pragma once

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

	private:
		BOOL SelectPhysicalDevice();
		BOOL CreateLogicalDevice();

	private:
		Common::Debug::LogFile* mpLogFile;

		VkInstance mInstance;
		VkSurfaceKHR mSurface;

		VkPhysicalDevice mPhysicalDevice;
		VkDevice mDevice;
	};
}

#include "Render/VK/Foundation/Core/Device.inl"