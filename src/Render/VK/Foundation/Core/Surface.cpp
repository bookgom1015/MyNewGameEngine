#include "Render/VK/Foundation/Core/pch_vk.h"
#include "Render/VK/Foundation/Core/Surface.hpp"
#include "Common/Debug/Logger.hpp"

using namespace Render::VK::Foundation::Core;

Surface::Surface() {}

Surface::~Surface() { CleanUp(); }

BOOL Surface::Initalize(Common::Debug::LogFile* const pLogFile, HWND hWnd, VkInstance instance) {
	mpLogFile = pLogFile;
	mhMainWnd = hWnd;
	mInstance = instance;

	CheckReturn(mpLogFile, CreateSurface());

	return TRUE;
}

BOOL Surface::CreateSurface() {
	VkWin32SurfaceCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hwnd = mhMainWnd;
	createInfo.hinstance = GetModuleHandle(nullptr);

	if (vkCreateWin32SurfaceKHR(mInstance, &createInfo, nullptr, &mSurface) != VK_SUCCESS)
		ReturnFalse(mpLogFile, L"Failed to create surface");

	return TRUE;
}

void Surface::CleanUp() {
	if (mSurface != VK_NULL_HANDLE)
		vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
}