#include "Render/DX/DxLowRenderer.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Core/HWInfo.hpp"
#include "Render/DX/Foundation/Core/Factory.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Foundation/Core/DescriptorHeap.hpp"
#include "Render/DX/Foundation/Core/SwapChain.hpp"
#include "Render/DX/Foundation/Core/DepthStencilBuffer.hpp"
#include "Render/DX/Foundation/Core/CommandObject.hpp"

#include <algorithm>

using namespace Microsoft::WRL;
using namespace Render::DX;

DxLowRenderer::DxLowRenderer() {
	mProcessor = std::make_unique<Common::Foundation::Core::Processor>();
	mFactory = std::make_unique<Foundation::Core::Factory>();
	mDevice = std::make_unique<Foundation::Core::Device>();
	mDescriptorHeap = std::make_unique<Foundation::Core::DescriptorHeap>();
	mSwapChain = std::make_unique<Foundation::Core::SwapChain>();
	mDepthStencilBuffer = std::make_unique<Foundation::Core::DepthStencilBuffer>();
	mCommandObject = std::make_unique<Foundation::Core::CommandObject>();
}

DxLowRenderer::~DxLowRenderer() {}

BOOL DxLowRenderer::Initialize(Common::Debug::LogFile* const pLogFile, HWND hWnd, UINT width, UINT height) {
	mhMainWnd = hWnd;
	mpLogFile = pLogFile;

	mClientWidth = width;
	mClientHeight = height;

	CheckReturn(mpLogFile, GetHWInfo());
	CheckReturn(mpLogFile, InitDirect3D(width, height));
	CheckReturn(mpLogFile, BuildDescriptors());

	CheckReturn(mpLogFile, mCommandObject->FlushCommandQueue());

	return TRUE;
}

void DxLowRenderer::CleanUp() {}

BOOL DxLowRenderer::GetHWInfo() {
	CheckReturn(mpLogFile, Common::Foundation::Core::HWInfo::GetCoreInfo(mpLogFile, *mProcessor.get()));

	return TRUE;
}

BOOL DxLowRenderer::CreateDescriptorHeaps() {
	CheckReturn(mpLogFile, mDescriptorHeap->CreateDescriptorHeaps(0, 0, 0));	

	return TRUE;
}

BOOL DxLowRenderer::OnResize(UINT width, UINT height) {
	mClientWidth = width;
	mClientHeight = height;

	CheckReturn(mpLogFile, mSwapChain->OnResize(mClientWidth, mClientHeight));
	CheckReturn(mpLogFile, mDepthStencilBuffer->OnResize(mClientWidth, mClientHeight));

	return TRUE;
}

BOOL DxLowRenderer::InitDirect3D(UINT width, UINT height) {
	CheckReturn(mpLogFile, mFactory->Initialize(mpLogFile));
	CheckReturn(mpLogFile, mFactory->SortAdapters());
	CheckReturn(mpLogFile, mFactory->SelectAdapter(mDevice.get()));
	CheckReturn(mpLogFile, mCommandObject->Initialize(mpLogFile, mDevice->GetDevice(), static_cast<UINT>(mProcessor->Logical)));
	CheckReturn(mpLogFile, mDescriptorHeap->Initialize(mpLogFile, mDevice->GetDevice(), mSwapChain.get(), mDepthStencilBuffer.get()));
	CheckReturn(mpLogFile, CreateSwapChain());
	CheckReturn(mpLogFile, CreateDepthStencilBuffer());
	CheckReturn(mpLogFile, CreateDescriptorHeaps());

	return TRUE;
}

BOOL DxLowRenderer::CreateSwapChain() {
	const auto initData = std::make_unique<Foundation::Core::SwapChain::InitData>();
	initData->LogFile = mpLogFile;
	initData->DxgiFactory = mFactory->DxgiFactory();
	initData->Device = mDevice->GetDevice();
	initData->CommandQueue = mCommandObject->CommandQueue();
	initData->MainWnd = mhMainWnd;
	initData->Width = mClientWidth;
	initData->Height = mClientHeight;
	initData->AllowTearing = mFactory->AllowTearing();

	CheckReturn(mpLogFile, mSwapChain->Initialize(initData.get()));

	return TRUE;
}

BOOL DxLowRenderer::CreateDepthStencilBuffer() {
	const auto initData = std::make_unique<Foundation::Core::DepthStencilBuffer::InitData>();
	initData->LogFile = mpLogFile;
	initData->Width = mClientWidth;
	initData->Height = mClientHeight;
	initData->Device = mDevice->GetDevice();

	CheckReturn(mpLogFile, mDepthStencilBuffer->Initialize(initData.get()));

	return TRUE;
}

BOOL DxLowRenderer::BuildDescriptors() {
	CheckReturn(mpLogFile, mDescriptorHeap->BuildDescriptors());

	const auto pDescHeap = mDescriptorHeap.get();
	CheckReturn(mpLogFile, mSwapChain->BuildDescriptors(pDescHeap));
	CheckReturn(mpLogFile, mDepthStencilBuffer->BuildDescriptors(pDescHeap));

	return TRUE;
}
