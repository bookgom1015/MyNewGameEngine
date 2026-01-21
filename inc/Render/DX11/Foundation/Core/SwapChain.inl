#ifndef __SWAPCHAIN_INL__
#define __SWAPCHAIN_INL__

namespace Render::DX11::Foundation::Core {
	constexpr D3D11_VIEWPORT& SwapChain::ScreenViewport() noexcept {
		return mScreenViewport;
	}

	constexpr const D3D11_VIEWPORT& SwapChain::ScreenViewport() const noexcept {
		return mScreenViewport;
	}

	ID3D11Texture2D* SwapChain::SwapChainBuffer() {
		return mSwapChainBuffer.Get();
	}

	ID3D11RenderTargetView* SwapChain::SwapChainBufferRtv() noexcept {
		return mhSwapChainBufferRtv.Get();
	}

	ID3D11Texture2D* SwapChain::SwapChainBufferCopy() {
		return mSwapChainBufferCopy.Get();
	}

	ID3D11ShaderResourceView* SwapChain::SwapChainBufferCopySrv() noexcept {
		return mhSwapChainBufferCopySrv.Get();
	}
}

#endif // __SWAPCHAIN_INL__