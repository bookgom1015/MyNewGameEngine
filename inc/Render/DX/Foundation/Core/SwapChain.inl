#ifndef __SWAPCHAIN_INL__
#define __SWAPCHAIN_INL__

Render::DX::Foundation::Resource::GpuResource* Render::DX::Foundation::Core::SwapChain::BackBuffer() const {
	return mSwapChainBuffers[mCurrBackBuffer].get();
}

D3D12_CPU_DESCRIPTOR_HANDLE Render::DX::Foundation::Core::SwapChain::BackBufferRtv() const {
	return mhBackBufferCpuRtvs[mCurrBackBuffer];
}

constexpr D3D12_VIEWPORT Render::DX::Foundation::Core::SwapChain::ScreenViewport() const {
	return mScreenViewport;
}

constexpr D3D12_RECT Render::DX::Foundation::Core::SwapChain::ScissorRect() const {
	return mScissorRect;
}

#endif // __SWAPCHAIN_INL__