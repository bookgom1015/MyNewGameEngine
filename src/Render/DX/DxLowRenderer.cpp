#include "Render/DX/DxLowRenderer.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Core/HWInfo.hpp"
#include "Render/DX/Foundation/Core/Factory.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Foundation/Core/DescriptorHeap.hpp"
#include "Render/DX/Foundation/Core/SwapChain.hpp"
#include "Render/DX/Foundation/Core/DepthStencilBuffer.hpp"
#include "Render/DX/Foundation/Core/CommandObject.hpp"
#include "Render/DX/Foundation/Util/D3D12Util.hpp"

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

	CheckReturn(mpLogFile, Foundation::Resource::GpuResource::Initialize(mpLogFile));
	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::Initialize(mpLogFile));

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
	const auto device = mDevice.get();

	CheckReturn(mpLogFile, mFactory->Initialize(mpLogFile));
	CheckReturn(mpLogFile, mFactory->SortAdapters());
	CheckReturn(mpLogFile, mFactory->SelectAdapter(device));

	CheckReturn(mpLogFile, mCommandObject->Initialize(mpLogFile, device, static_cast<UINT>(mProcessor->Logical)));

	CheckReturn(mpLogFile, mDescriptorHeap->Initialize(mpLogFile, device, mSwapChain.get(), mDepthStencilBuffer.get()));

	CheckReturn(mpLogFile, CreateSwapChain());
	CheckReturn(mpLogFile, CreateDepthStencilBuffer());
	CheckReturn(mpLogFile, CreateDescriptorHeaps());

	return TRUE;
}

BOOL DxLowRenderer::CreateSwapChain() {
	auto initData = Foundation::Core::SwapChain::MakeInitData();
	initData->Factory = mFactory.get();
	initData->Device = mDevice.get();
	initData->CommandObject = mCommandObject.get();
	initData->MainWnd = mhMainWnd;
	initData->Width = mClientWidth;
	initData->Height = mClientHeight;
	initData->AllowTearing = mFactory->AllowTearing();

	CheckReturn(mpLogFile, mSwapChain->Initialize(mpLogFile, initData.get()));

	return TRUE;
}

BOOL DxLowRenderer::CreateDepthStencilBuffer() {
	auto initData = Foundation::Core::DepthStencilBuffer::MakeInitData();
	initData->Width = mClientWidth;
	initData->Height = mClientHeight;
	initData->Device = mDevice.get();

	CheckReturn(mpLogFile, mDepthStencilBuffer->Initialize(mpLogFile, initData.get()));

	return TRUE;
}

BOOL DxLowRenderer::BuildDescriptors() {
	CheckReturn(mpLogFile, mDescriptorHeap->BuildDescriptors());

	const auto pDescHeap = mDescriptorHeap.get();
	CheckReturn(mpLogFile, mSwapChain->BuildDescriptors(pDescHeap));
	CheckReturn(mpLogFile, mDepthStencilBuffer->BuildDescriptors(pDescHeap));

	return TRUE;
}
