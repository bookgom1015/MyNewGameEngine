#ifndef __SWAPCHAIN_INL__
#define __SWAPCHAIN_INL__

namespace Render::DX11::Foundation::Core {
	constexpr D3D11_VIEWPORT& SwapChain::ScreenViewport() noexcept {
		return mScreenViewport;
	}

	constexpr const D3D11_VIEWPORT& SwapChain::ScreenViewport() const noexcept {
		return mScreenViewport;
	}

	ID3D11Texture2D* SwapChain::BackBuffer() {
		return mSwapChainBuffer.Get();
	}

	ID3D11RenderTargetView* SwapChain::BackBufferRtv() noexcept {
		return mhSwapChainBufferRtv.Get();
	}

	ID3D11Texture2D* SwapChain::SceneMap() {
		return mSceneMap.Get();
	}

	ID3D11RenderTargetView* SwapChain::SceneMapRtv() noexcept {
		return mhSceneMapRtv.Get();
	}

	ID3D11ShaderResourceView* SwapChain::SceneMapSrv() noexcept {
		return mhSceneMapSrv.Get();
	}

	ID3D11Texture2D* SwapChain::SceneMapCopy() {
		return mSceneMapCopy.Get();
	}

	ID3D11ShaderResourceView* SwapChain::SceneMapCopySrv() noexcept {
		return mhSceneMapCopySrv.Get();
	}
}

#endif // __SWAPCHAIN_INL__