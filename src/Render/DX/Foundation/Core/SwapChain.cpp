#include "Render/DX/Foundation/Core/SwapChain.hpp"
#include "Render/DX/Foundation/Core/Factory.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Foundation/Core/CommandObject.hpp"
#include "Render/DX/Foundation/Core/DescriptorHeap.hpp"
#include "Render/DX/Foundation/Resource/GpuResource.hpp"
#include "Render/DX/Foundation/Resource/FrameResource.hpp"
#include "Render/DX/Foundation/Util/D3D12Util.hpp"

using namespace Render::DX::Foundation::Core;

SwapChain::SwapChain() {
	mInitData = {};
	for (UINT i = 0; i < SwapChainBufferCount; ++i)
		mSwapChainBuffers[i] = std::make_unique<Resource::GpuResource>();
	mhBackBufferCpuSrvs = {};
	mhBackBufferGpuSrvs = {};
	mhBackBufferCpuRtvs = {};
}

SwapChain::~SwapChain() {}

UINT SwapChain::CbvSrvUavDescCount() const { return SwapChainBufferCount; }
UINT SwapChain::RtvDescCount() const { return SwapChainBufferCount; }
UINT SwapChain::DsvDescCount() const { return 0; }

SwapChain::InitDataPtr SwapChain::MakeInitData() {
	return std::unique_ptr<InitData>(new InitData());
}

BOOL SwapChain::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	mScreenViewport = { 0, 0, static_cast<FLOAT>(mInitData.Width), static_cast<FLOAT>(mInitData.Height), 0.f, 1.f };
	mScissorRect = { 0,0,static_cast<LONG>(mInitData.Width), static_cast<LONG>(mInitData.Height) };

	CheckReturn(mpLogFile, CreateSwapChain());
	CheckReturn(mpLogFile, BuildSwapChainBuffers());

	return TRUE;
}

BOOL SwapChain::BuildDescriptors(DescriptorHeap* const pDescHeap) {
	for (UINT i = 0; i < SwapChainBufferCount; ++i) {
		UINT offset = i == 0 ? 0 : 1;
		mhBackBufferCpuSrvs[i] = pDescHeap->CbvSrvUavCpuOffset(offset);
		mhBackBufferGpuSrvs[i] = pDescHeap->CbvSrvUavGpuOffset(offset);
		mhBackBufferCpuRtvs[i] = pDescHeap->RtvCpuOffset(offset);
	}

	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL SwapChain::ReadyToPresent(Resource::FrameResource* const pFrameResource) {
	CheckReturn(mpLogFile, mInitData.CommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0));
	const auto cmdList = mInitData.CommandObject->CommandList(0);

	mSwapChainBuffers[mCurrBackBuffer]->Transite(cmdList, D3D12_RESOURCE_STATE_PRESENT);

	CheckReturn(mpLogFile, mInitData.CommandObject->ExecuteCommandList(0));

	return TRUE;
}

BOOL SwapChain::Present(BOOL bAllowTearing) {
	CheckHRESULT(mpLogFile, mSwapChain->Present(0, bAllowTearing ? DXGI_PRESENT_ALLOW_TEARING : 0));

	return TRUE;
}

void SwapChain::NextBackBuffer() {
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;
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
	CheckReturn(mpLogFile, Util::D3D12Util::CreateSwapChain(mInitData.Factory, mInitData.CommandObject, &sd, &mSwapChain));

	return TRUE;
}

BOOL SwapChain::BuildSwapChainBuffers() {
	// Resize the previous resources we will be creating.
	for (UINT i = 0; i < SwapChainBufferCount; ++i)
		mSwapChainBuffers[i]->Reset();

	// Resize the swap chain.
	CheckHRESULT(mpLogFile, mSwapChain->ResizeBuffers(
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
		Util::D3D12Util::CreateShaderResourceView(mInitData.Device, backBuffer, &srvDesc, mhBackBufferCpuSrvs[i]);
		Util::D3D12Util::CreateRenderTargetView(mInitData.Device, backBuffer, nullptr, mhBackBufferCpuRtvs[i]);
	}

	return TRUE;
}

BOOL SwapChain::OnResize(UINT width, UINT height) {
	mInitData.Width = width;
	mInitData.Height = height;

	mScreenViewport = { 0, 0, static_cast<FLOAT>(mInitData.Width), static_cast<FLOAT>(mInitData.Height), 0.f, 1.f };
	mScissorRect = { 0,0,static_cast<LONG>(mInitData.Width), static_cast<LONG>(mInitData.Height) };

	CheckReturn(mpLogFile, BuildSwapChainBuffers());
	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}