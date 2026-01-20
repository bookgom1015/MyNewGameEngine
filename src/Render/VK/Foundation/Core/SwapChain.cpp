#include "Render/VK/Foundation/Core/pch_vk.h"
#include "Render/VK/Foundation/Core/SwapChain.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/VK/Foundation/Util/VulkanUtil.hpp"

using namespace Render::VK::Foundation::Core;

SwapChain::SwapChain() {}

SwapChain::~SwapChain() { CleanUp(); }

BOOL SwapChain::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	mScreenViewport = {
		0.f, 0.f,
		static_cast<float>(mInitData.Width),
		static_cast<float>(mInitData.Height),
		0.f, 1.f
	};

	mScissorRect = {
		{ 0, 0 },
		{ mInitData.Width, mInitData.Height }
	};

	CheckReturn(mpLogFile, CreateSwapChain());
	CheckReturn(mpLogFile, CreateImageViews());

	return TRUE;
}

void SwapChain::CleanUp() {
	for (UINT i = 0; i < SwapChainImageCount; ++i) {
		const auto swapChainImageView = mSwapChainImageViews[i];
		if (swapChainImageView != VK_NULL_HANDLE)
			vkDestroyImageView(mInitData.Device, swapChainImageView, nullptr);
	}
	
	if (mSwapChain != VK_NULL_HANDLE)
		vkDestroySwapchainKHR(mInitData.Device, mSwapChain, nullptr);
}

BOOL SwapChain::OnResize(UINT width, UINT height) {
	mInitData.Width = width;
	mInitData.Height = height;

	mScreenViewport = {
		0.f, 0.f,
		static_cast<float>(mInitData.Width),
		static_cast<float>(mInitData.Height),
		0.f, 1.f
	};

	mScissorRect = {
		{ 0, 0 },
		{ mInitData.Width, mInitData.Height }
	};

	CheckReturn(mpLogFile, CreateSwapChain());
	CheckReturn(mpLogFile, CreateImageViews());

	return TRUE;
}

BOOL SwapChain::CreateSwapChain() {
	Foundation::Util::SwapChainSupportDetails swapChainSupport = Foundation::Util::VulkanUtil::QuerySwapChainSupport(mInitData.PhysicalDevice, mInitData.Surface);

	VkSurfaceFormatKHR surfaceFormat = Foundation::Util::VulkanUtil::ChooseSwapSurfaceFormat(swapChainSupport.Formats);
	VkPresentModeKHR presentMode = Foundation::Util::VulkanUtil::ChooseSwapPresentMode(swapChainSupport.PresentModes);
	VkExtent2D extent = Foundation::Util::VulkanUtil::ChooseSwapExtent(mInitData.Width, mInitData.Height, swapChainSupport.Capabilities);

	UINT imageCount = SwapChainImageCount;

	if (imageCount < swapChainSupport.Capabilities.minImageCount ||
		(swapChainSupport.Capabilities.maxImageCount > 0 && imageCount > swapChainSupport.Capabilities.maxImageCount)) {
		ReturnFalse(mpLogFile, L"Swap-chain does not support the specified number of images");
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = mInitData.Surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	Foundation::Util::QueueFamilyIndices indices = Foundation::Util::QueueFamilyIndices::FindQueueFamilies(mInitData.PhysicalDevice, mInitData.Surface);
	UINT queueFamilyIndices[] = {
		indices.GetGraphicsFamilyIndex(),
		indices.GetPresentFamilyIndex()
	};

	if (indices.GraphicsFamily != indices.PresentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 1;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	createInfo.preTransform = swapChainSupport.Capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(mInitData.Device, &createInfo, nullptr, &mSwapChain) != VK_SUCCESS)
		ReturnFalse(mpLogFile, L"Failed to create swap chain");

	if (vkGetSwapchainImagesKHR(mInitData.Device, mSwapChain, &imageCount, mSwapChainImages.data()) != VK_SUCCESS)
		ReturnFalse(mpLogFile, L"Failed to get swap chain images");

	mSwapChainImageFormat = surfaceFormat.format;
	mSwapChainExtent = extent;

	return TRUE;
}

BOOL SwapChain::CreateImageViews() {
	for (size_t i = 0, end = mSwapChainImages.size(); i < end; ++i) {
		CheckReturn(mpLogFile, Foundation::Util::VulkanUtil::CreateImageView(
			mpLogFile,
			mInitData.Device,
			mSwapChainImages[i],
			mSwapChainImageFormat,
			1,
			VK_IMAGE_ASPECT_COLOR_BIT,
			mSwapChainImageViews[i]
		));
	}

	return TRUE;
}
