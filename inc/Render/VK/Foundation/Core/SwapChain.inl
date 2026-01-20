#ifndef __SWAPCHAIN_INL__
#define __SWAPCHAIN_INL__

namespace Render::VK::Foundation::Core {
	const VkViewport& SwapChain::ScreenViewport() const { return mScreenViewport; }

	const VkRect2D& SwapChain::ScissorRect() const { return mScissorRect; }
}

#endif // __SWAPCHAIN_INL__