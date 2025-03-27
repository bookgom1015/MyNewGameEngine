#include "Render/VK/VkLowRenderer.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/VK/Foundation/Core/Instance.hpp"
#include "Render/VK/Foundation/Core/Surface.hpp"
#include "Render/VK/Foundation/Core/Device.hpp"
#include "Render/VK/Foundation/Core/SwapChain.hpp"
#include "Render/VK/Foundation/Core/CommandObject.hpp"
#include "Render/VK/Foundation/Util/VulkanUtil.hpp"

#pragma warning(disable: 26495)

#include <algorithm>
#include <set>

using namespace Render::VK;

VkLowRenderer::VkLowRenderer() {
	mInstance = std::make_unique<Foundation::Core::Instance>();
	mSurface = std::make_unique<Foundation::Core::Surface>();
	mDevice = std::make_unique<Foundation::Core::Device>();
	mSwapChain = std::make_unique<Foundation::Core::SwapChain>();
	mCommandObject = std::make_unique<Foundation::Core::CommandObject>();
}

VkLowRenderer::~VkLowRenderer() {}

BOOL VkLowRenderer::Initialize(Common::Debug::LogFile* const pLogFile, HWND hWnd, UINT width, UINT height) {
	mhMainWnd = hWnd;
	mpLogFile = pLogFile;

	mClientWidth = width;
	mClientHeight = height;

	CheckReturn(mpLogFile, CreateInstance());	
	CheckReturn(mpLogFile, CreateSurface());
	CheckReturn(mpLogFile, CreateDevice());
	CheckReturn(mpLogFile, CreateSwapChain());
	CheckReturn(mpLogFile, CreateCommandObjects());

	return TRUE;
}

void VkLowRenderer::CleanUp() {	
	mCommandObject->CleanUp();
	mSwapChain->CleanUp();
	mDevice->CleanUp();
	mSurface->CleanUp();
	mInstance->CleanUp();
}

BOOL VkLowRenderer::OnResize(UINT width, UINT height) {
	CheckReturn(mpLogFile, mSwapChain->OnResize(width, height));

	return TRUE;
}

BOOL VkLowRenderer::CreateInstance() {
	CheckReturn(mpLogFile, mInstance->Initalize(mpLogFile));

	return TRUE;
}

BOOL VkLowRenderer::CreateSurface() {
	CheckReturn(mpLogFile, mSurface->Initalize(mpLogFile, mhMainWnd, mInstance->GetInstance()));
	
	return TRUE;
}

BOOL VkLowRenderer::CreateDevice() {
	CheckReturn(mpLogFile, mDevice->Initialize(mpLogFile, mInstance->GetInstance(), mSurface->GetSurface()));

	return TRUE;
}

BOOL VkLowRenderer::CreateSwapChain() {
	Foundation::Core::SwapChain::InitData initData;
	initData.LogFile = mpLogFile;
	initData.PhysicalDevice = mDevice->PhysicalDevice();
	initData.Device = mDevice->LogicalDevice();
	initData.Surface = mSurface->GetSurface();
	initData.Width = mClientWidth;
	initData.Height = mClientHeight;

	mSwapChain->Initialize(reinterpret_cast<void*>(&initData));

	return TRUE;
}

BOOL VkLowRenderer::CreateCommandObjects() {
	CheckReturn(mpLogFile, mCommandObject->Initialize(
		mpLogFile, 
		mDevice->PhysicalDevice(), 
		mDevice->LogicalDevice(),
		mSurface->GetSurface(),
		Foundation::Core::SwapChain::SwapChainImageCount));

	return TRUE;
}