#pragma once

#pragma comment(lib, "vulkan-1.lib")

#include "Common/Foundation/Core/pch_common.h"

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>