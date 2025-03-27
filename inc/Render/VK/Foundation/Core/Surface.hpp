#pragma once

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

namespace Common::Debug {
	struct LogFile;
}

namespace Render::VK::Foundation::Core {
	class Surface {
	public:
		Surface() = default;
		virtual ~Surface();

	public:
		BOOL Initalize(Common::Debug::LogFile* const pLogFile, HWND hWnd, VkInstance instance);
		void CleanUp();

	public:
		__forceinline VkSurfaceKHR GetSurface();

	private:
		BOOL CreateSurface();

	private:
		Common::Debug::LogFile* mpLogFile = nullptr;

		HWND mhMainWnd;

		VkInstance mInstance;

		VkSurfaceKHR mSurface;
	};
}

#include "Render/VK/Foundation/Core/Surface.inl"