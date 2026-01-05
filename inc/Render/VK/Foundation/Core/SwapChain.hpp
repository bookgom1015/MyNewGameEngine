#pragma once

#include "Render/VK/Foundation/ShadingObject.hpp"

namespace Render::VK::Foundation::Core {
	class SwapChain : public ShadingObject {
	public:
		static const UINT SwapChainImageCount = 3;

	public:
		struct InitData {
			VkPhysicalDevice PhysicalDevice;
			VkDevice Device;
			VkSurfaceKHR Surface;
			UINT Width; 
			UINT Height;
		};

	public:
		SwapChain() = default;
		virtual ~SwapChain();

	public:
		__forceinline const VkViewport& ScreenViewport() const;
		__forceinline const VkRect2D& ScissorRect() const;

	public:
		virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) override;
		virtual void CleanUp() override;

	public:
		virtual BOOL OnResize(UINT width, UINT height) override;

	private:
		BOOL CreateSwapChain();
		BOOL CreateImageViews();

	private:
		InitData mInitData{};

		VkSwapchainKHR mSwapChain{};
		VkFormat mSwapChainImageFormat{};
		VkExtent2D mSwapChainExtent{};

		std::array<VkImage, SwapChainImageCount> mSwapChainImages{};
		std::array<VkImageView, SwapChainImageCount> mSwapChainImageViews{};

		VkViewport mScreenViewport{};
		VkRect2D mScissorRect{};
	};
}

namespace Render::VK::Foundation::Core {
	const VkViewport& SwapChain::ScreenViewport() const { return mScreenViewport; }

	const VkRect2D& SwapChain::ScissorRect() const { return mScissorRect; }
}