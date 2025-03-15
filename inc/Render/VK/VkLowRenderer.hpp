#pragma once

#pragma comment(lib, "vulkan-1.lib")

#include <array>

#ifndef VK_USE_PLATFORM_WIN32_KHR
	#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

#include "Common/Render/Renderer.hpp"

namespace Render::VK {
	const UINT SwapChainImageCount = 3;

	class VkLowRenderer {

	protected:
		VkLowRenderer();
		virtual ~VkLowRenderer();

	protected: // Functions that called only once
		virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, HWND hWnd, UINT width, UINT height);
		virtual void CleanUp();

	protected: // Functions that called whenever a message is called
		virtual BOOL OnResize(UINT width, UINT height);

	private:
		BOOL CreateInstance();
		BOOL CreateSurface();
		BOOL SelectPhysicalDevice();
		BOOL CreateLogicalDevice();
		BOOL CreateSwapChain();
		BOOL CreateImageViews();

	protected:
		Common::Debug::LogFile* mpLogFile;

		HWND mhMainWnd = NULL;

		UINT mClientWidth = 0;
		UINT mClientHeight = 0;

	protected:
		VkInstance mInstance;
		VkSurfaceKHR mSurface;
		VkPhysicalDevice mPhysicalDevice;
		VkDevice mDevice;
		VkSwapchainKHR mSwapChain;
		VkFormat mSwapChainImageFormat;
		VkExtent2D mSwapChainExtent;
		std::array<VkImage, SwapChainImageCount> mSwapChainImages;
		std::array<VkImageView, SwapChainImageCount> mSwapChainImageViews;

		VkDebugUtilsMessengerEXT mDebugMessenger;

		VkQueue mGraphicsQueue;
		VkQueue mPresentQueue;
	};
}