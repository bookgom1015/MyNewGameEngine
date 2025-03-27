#pragma once

#include <array>

#include <Windows.h>

#ifndef VK_USE_PLATFORM_WIN32_KHR
	#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

#include "Common/Debug/Logger.hpp"

namespace Render::VK::Foundation {
	class ShadingObject {
	public:
		virtual BOOL Initialize(void* const pInitData) = 0;
		virtual void CleanUp() = 0;

	public:
		virtual BOOL OnResize(UINT width, UINT height);
	};
}