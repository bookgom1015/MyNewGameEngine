#ifndef __SWAPCHAIN_INL__
#define __SWAPCHAIN_INL__

namespace Render::DX::Foundation::Core {
	Render::DX::Foundation::Resource::GpuResource* SwapChain::BackBuffer() const {
		return mSwapChainBuffers[mCurrBackBuffer].get();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE SwapChain::BackBufferRtv() const {
		return mhBackBufferCpuRtvs[mCurrBackBuffer];
	}

	Render::DX::Foundation::Resource::GpuResource* SwapChain::SceneMap() const {
		return mSceneMap.get();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE SwapChain::SceneMapRtv() const {
		return mhSceneMapCpuRtv;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE SwapChain::SceneMapSrv() const {
		return mhSceneMapGpuSrv;
	}

	Render::DX::Foundation::Resource::GpuResource* SwapChain::SceneMapCopy() const {
		return mSceneMapCopy.get();
	}

	D3D12_GPU_DESCRIPTOR_HANDLE SwapChain::SceneMapCopySrv() const {
		return mhSceneMapCopyGpuSrv;
	}

	constexpr D3D12_VIEWPORT SwapChain::ScreenViewport() const {
		return mScreenViewport;
	}

	constexpr D3D12_RECT SwapChain::ScissorRect() const {
		return mScissorRect;
	}
}

#endif // __SWAPCHAIN_INL__