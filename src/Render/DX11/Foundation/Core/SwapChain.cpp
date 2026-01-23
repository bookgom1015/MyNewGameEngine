#include "Render/DX11/Foundation/Core/pch_d3d11.h"
#include "Render/DX11/Foundation/Core/SwapChain.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX11/Foundation/Core/Device.hpp"

using namespace Render::DX11::Foundation::Core;

std::unique_ptr<SwapChain::InitData> SwapChain::MakeInitData() {
	return std::unique_ptr<InitData>(new InitData());
}

SwapChain::SwapChain() {}

SwapChain::~SwapChain() { CleanUp(); }

BOOL SwapChain::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	mScreenViewport = {
		.TopLeftX = 0,
		.TopLeftY = 0,
		.Width = static_cast<FLOAT>(mInitData.Width),
		.Height = static_cast<FLOAT>(mInitData.Height),
		.MinDepth = 0.f,
		.MaxDepth = 1.f
	};

	CheckReturn(mpLogFile, CreateSwapChain());
	CheckReturn(mpLogFile, CreateSwapChainBuffer());
	CheckReturn(mpLogFile, CreateSwapChainBufferView());

	return TRUE;
}

void SwapChain::CleanUp() {
	if (mbCleanedUp) return;

	mhSwapChainBufferCopySrv.Reset();
	mSwapChainBufferCopy.Reset();

	mhSwapChainBufferRtv.Reset();
	mSwapChainBuffer.Reset();

	mSwapChain.Reset();

	mbCleanedUp = TRUE;
}

BOOL SwapChain::OnResize(UINT width, UINT height) {
	mInitData.Width = width;
	mInitData.Height = height;

	mScreenViewport = {
		.TopLeftX = 0,
		.TopLeftY = 0,
		.Width = static_cast<FLOAT>(mInitData.Width),
		.Height = static_cast<FLOAT>(mInitData.Height),
		.MinDepth = 0.f,
		.MaxDepth = 1.f
	};

	CheckReturn(mpLogFile, CreateSwapChainBuffer());
	CheckReturn(mpLogFile, CreateSwapChainBufferView());

	return TRUE;
}

BOOL SwapChain::Present() {
	CheckHRESULT(mpLogFile, mSwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING));
	return TRUE;
}

BOOL SwapChain::CreateSwapChain() {
	DXGI_SWAP_CHAIN_DESC1 desc{};
	desc.Width = mInitData.Width;
	desc.Height = mInitData.Height;
	desc.Format = ShadingConvention::SwapChain::BackBufferFormat;
	desc.Stereo = FALSE;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount = SwapChainBufferCount;
	desc.Scaling = DXGI_SCALING_STRETCH;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	CheckReturn(mpLogFile, mInitData.Device->CreateSwapChainForHwnd(
		mInitData.WindowHandle,
		&desc,
		nullptr,
		nullptr,
		&mSwapChain));

	return TRUE;
}

BOOL SwapChain::CreateSwapChainBuffer() {
	mhSwapChainBufferRtv.Reset();
	mSwapChainBuffer.Reset();

	CheckHRESULT(mpLogFile, mSwapChain->ResizeBuffers(
		SwapChainBufferCount,
		mInitData.Width, mInitData.Height,
		ShadingConvention::SwapChain::BackBufferFormat,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING));

	CheckHRESULT(mpLogFile, mSwapChain->GetBuffer(0, IID_PPV_ARGS(&mSwapChainBuffer)));

	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = mInitData.Width;
	desc.Height = mInitData.Height;
	desc.Format = ShadingConvention::SwapChain::BackBufferFormat;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	CheckReturn(mpLogFile, mInitData.Device->CreateTexture2D(
		&desc, nullptr, &mSwapChainBufferCopy));

	return TRUE;
}

BOOL SwapChain::CreateSwapChainBufferView() {
	CheckReturn(mpLogFile, mInitData.Device->CreateRenderTargetView(
		mSwapChainBuffer.Get(), nullptr, &mhSwapChainBufferRtv));

	CheckReturn(mpLogFile, mInitData.Device->CreateShaderResourceView(
		mSwapChainBufferCopy.Get(), nullptr, &mhSwapChainBufferCopySrv));

	return TRUE;
}