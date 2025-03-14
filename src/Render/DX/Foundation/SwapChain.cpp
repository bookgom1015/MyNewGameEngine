#include "Render/DX/Foundation/SwapChain.hpp"

using namespace Render::DX::Foundation;

SwapChain::SwapChain() {
	mInitData = {};
	for (UINT i = 0; i < SwapChainBufferCount; ++i)
		mSwapChainBuffers[i] = std::make_unique<GpuResource>();
	mhBackBufferCpuSrvs = {};
	mhBackBufferGpuSrvs = {};
	mhBackBufferCpuRtvs = {};
}

SwapChain::~SwapChain() {}

UINT SwapChain::CbvSrvUavDescCount() const { return SwapChainBufferCount; }
UINT SwapChain::RtvDescCount() const { return SwapChainBufferCount; }
UINT SwapChain::DsvDescCount() const { return 0; }

BOOL SwapChain::Initialize(void* const pData) {
	if (pData == nullptr) return FALSE;

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	CheckReturn(mInitData.LogFile, CreateSwapChain());
	CheckReturn(mInitData.LogFile, BuildSwapChainBuffers());

	return TRUE;
}

void SwapChain::CleanUp() {

}

BOOL SwapChain::BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuCbvSrvUav, CD3DX12_GPU_DESCRIPTOR_HANDLE& hGpuCbvSrvUav,
		CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuRtv, CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuDsv,
		UINT cbvSrvUavDescSize, UINT rtvDescSize, UINT dsvDescSize) {
	for (UINT i = 0; i < SwapChainBufferCount; ++i) {
		UINT offset = i == 0 ? 0 : 1;
		mhBackBufferCpuSrvs[i] = hCpuCbvSrvUav.Offset(offset, cbvSrvUavDescSize);
		mhBackBufferGpuSrvs[i] = hGpuCbvSrvUav.Offset(offset, cbvSrvUavDescSize);
		mhBackBufferCpuRtvs[i] = hCpuRtv.Offset(offset, rtvDescSize);
	}

	CheckReturn(mInitData.LogFile, BuildDescriptors());

	return TRUE;
}

BOOL SwapChain::CreateSwapChain() {
	// Release the previous swapchain we will be recreating.
	mSwapChain.Reset();

	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = mInitData.Width;
	sd.BufferDesc.Height = mInitData.Height;
	sd.BufferDesc.RefreshRate.Numerator = 0;
	sd.BufferDesc.RefreshRate.Denominator = 0;
	sd.BufferDesc.Format = ShadingConvention::SwapChain::BackBufferFormat;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = SwapChainBufferCount;
	sd.OutputWindow = mInitData.MainWnd;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	// Note: Swap chain uses queue to perfrom flush.
	CheckHRESULT(mInitData.LogFile, mInitData.DxgiFactory->CreateSwapChain(mInitData.CommandQueue, &sd, &mSwapChain));

	return TRUE;
}

BOOL SwapChain::BuildSwapChainBuffers() {
	// Resize the previous resources we will be creating.
	for (UINT i = 0; i < SwapChainBufferCount; ++i)
		mSwapChainBuffers[i]->Reset();

	// Resize the swap chain.
	CheckHRESULT(mInitData.LogFile, mSwapChain->ResizeBuffers(
		SwapChainBufferCount,
		mInitData.Width,
		mInitData.Height,
		ShadingConvention::SwapChain::BackBufferFormat,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | (mInitData.AllowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0)));

	for (UINT i = 0; i < SwapChainBufferCount; ++i) {
		const auto buffer = mSwapChainBuffers[i].get();
		buffer->OnResize(mSwapChain.Get(), i);
	}

	mCurrBackBuffer = 0;

	return TRUE;
}

BOOL SwapChain::BuildDescriptors() {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = ShadingConvention::SwapChain::BackBufferFormat;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.f;
	srvDesc.Texture2D.MipLevels = 1;

	for (UINT i = 0; i < SwapChainBufferCount; ++i) {
		const auto backBuffer = mSwapChainBuffers[i]->Resource();
		mInitData.Device->CreateShaderResourceView(backBuffer, &srvDesc, mhBackBufferCpuSrvs[i]);
		mInitData.Device->CreateRenderTargetView(backBuffer, nullptr, mhBackBufferCpuRtvs[i]);
	}

	return TRUE;
}

BOOL SwapChain::OnResize(UINT width, UINT height) {
	mInitData.Width = width;
	mInitData.Height = height;

	CheckReturn(mInitData.LogFile, BuildSwapChainBuffers());
	CheckReturn(mInitData.LogFile, BuildDescriptors());

	return TRUE;
}