#include "Render/DX11/Foundation/Core/pch_d3d11.h"
#include "Render/DX11/Dx11LowRenderer.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Core/HWInfo.hpp"
#include "Common/Foundation/Core/WindowsManager.hpp"
#include "Render/DX11/Foundation/Core/Device.hpp"
#include "Render/DX11/Foundation/Core/SwapChain.hpp"
#include "Render/DX11/Foundation/Core/DepthStencilBuffer.hpp"
#include "ImGuiManager/DX11/Dx11ImGuiManager.hpp"

using namespace Render::DX11;

Dx11LowRenderer::Dx11LowRenderer() {
	mProcessor = std::make_unique<Common::Foundation::Core::Processor>();
	mDevice = std::make_unique<Foundation::Core::Device>();
	mSwapChain = std::make_unique<Foundation::Core::SwapChain>();
	mDepthStencilBuffer = std::make_unique<Foundation::Core::DepthStencilBuffer>();
}

Dx11LowRenderer::~Dx11LowRenderer() {}

BOOL Dx11LowRenderer::Initialize(
		Common::Debug::LogFile* const pLogFile,
		Common::Foundation::Core::WindowsManager* const pWndManager,
		Common::ImGuiManager::ImGuiManager* const pImGuiManager,
		Common::Render::ShadingArgument::ShadingArgumentSet* const pShadingArgSet,
		UINT width, UINT height) {
	mpLogFile = pLogFile;
	mClientWidth = width;
	mClientHeight = height;
	mpWindowsManager = pWndManager;
	mpShadingArgumentSet = pShadingArgSet;

	mpImGuiManager = static_cast<ImGuiManager::DX11::Dx11ImGuiManager*>(pImGuiManager);

	CheckReturn(mpLogFile, Common::Foundation::Core::HWInfo::GetCoreInfo(mpLogFile, *mProcessor.get()));

	CheckReturn(mpLogFile, mDevice->Initialize(pLogFile));
	// SwapChain
	{
		decltype(auto) initData = Foundation::Core::SwapChain::MakeInitData();
		initData->Width = width;
		initData->Height = height;
		initData->WindowHandle = pWndManager->MainWindowHandle();
		initData->Device = mDevice.get();
		CheckReturn(mpLogFile, mSwapChain->Initialize(pLogFile, initData.get()));
	}
	// DepthStencilBuffer
	{
		decltype(auto) initData = Foundation::Core::DepthStencilBuffer::MakeInitData();
		initData->Width = width;
		initData->Height = height;
		initData->Device = mDevice.get();
		CheckReturn(mpLogFile, mDepthStencilBuffer->Initialize(pLogFile, initData.get()));
	}

	return TRUE;
}

void Dx11LowRenderer::CleanUp() {
	if (mDepthStencilBuffer) {
		mDepthStencilBuffer->CleanUp();
		mDepthStencilBuffer.reset();
	}
	if (mSwapChain) {
		mSwapChain->CleanUp();
		mSwapChain.reset();
	}
	if (mDevice) {
		mDevice->CleanUp();
		mDevice.reset();
	}

	mbCleanedUp = TRUE;
}

BOOL Dx11LowRenderer::OnResize(UINT width, UINT height) {
	mClientWidth = width;
	mClientHeight = height;

	CheckReturn(mpLogFile, mSwapChain->OnResize(width, height));
	CheckReturn(mpLogFile, mDepthStencilBuffer->OnResize(width, height));

	return TRUE;
}

BOOL Dx11LowRenderer::Update(FLOAT deltaTime) {
	return TRUE;
}

BOOL Dx11LowRenderer::Draw() {
	return TRUE;
}