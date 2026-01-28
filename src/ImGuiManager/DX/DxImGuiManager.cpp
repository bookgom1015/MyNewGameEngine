#include "ImGuiManager/DX/pch_imgui_dx.h"
#include "ImGuiManager/DX/DxImGuiManager.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Light.h"
#include "Common/Render/ShadingArgument.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Foundation/Core/CommandObject.hpp"
#include "Render/DX/Foundation/Core/DescriptorHeap.hpp"
#include "Render/DX/Foundation/Core/SwapChain.hpp"
#include "Render/DX/Foundation/Resource/FrameResource.hpp"
#include "Render/DX/Foundation/Resource/GpuResource.hpp"

using namespace ImGuiManager::DX;
using namespace DirectX;

extern "C" ImGuiManagerAPI Common::ImGuiManager::ImGuiManager* ImGuiManager::CreateImGuiManager() {
	return new DxImGuiManager();
}

extern "C" ImGuiManagerAPI void ImGuiManager::DestroyImGuiManager(Common::ImGuiManager::ImGuiManager* const imGuiManager) {
	delete imGuiManager;
}

DxImGuiManager::DxImGuiManager() {}

DxImGuiManager::~DxImGuiManager() {}

BOOL DxImGuiManager::InitializeD3D12(
		Render::DX::Foundation::Core::Device* const pDevice, 
		Render::DX::Foundation::Core::DescriptorHeap* const pDescriptorHeap) {
	ImGui::SetCurrentContext(mpContext);

	mhImGuiCpuSrv = pDescriptorHeap->CbvSrvUavCpuOffset(1);
	mhImGuiGpuSrv = pDescriptorHeap->CbvSrvUavGpuOffset(1);
	
	CheckReturn(mpLogFile, ImGui_ImplDX12_Init(
		pDevice->md3dDevice.Get(),
		Render::DX::Foundation::Core::SwapChain::SwapChainBufferCount,
		ShadingConvention::SwapChain::BackBufferFormat,
		pDescriptorHeap->mCbvSrvUavHeap.Get(),
		mhImGuiCpuSrv,
		mhImGuiGpuSrv));

	mbIsD3D12Initialized = TRUE;

	return TRUE;
}

void DxImGuiManager::CleanUp() {	
	if (mbIsD3D12Initialized) ImGui_ImplDX12_Shutdown();
	ImGuiManager::CleanUp();
}

BOOL DxImGuiManager::DrawImGui(
		ID3D12GraphicsCommandList6* const pCmdList,
		Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet,
		Common::Foundation::Light* lights[],
		UINT numLights,
		std::queue<std::shared_ptr<Common::Foundation::Light>>& pendingLights,
		UINT clientWidth, UINT clientHeight,
		BOOL bRaytracingSupported) {
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	const auto Width = static_cast<FLOAT>(clientWidth);
	const auto Height = static_cast<FLOAT>(clientHeight);

	// Setup window properties
	{
		ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiCond_Once);

		const ImVec2 MinSize(300.f, Height);
		const ImVec2 MaxSize(std::max(Width, 300.f), Height);
		ImGui::SetNextWindowSizeConstraints(MinSize, MaxSize);

		//ImGui::SetNextWindowSize(ImVec2(450.f, static_cast<FLOAT>(clientHeight)), ImGuiCond_Always);
		ImGui::SetNextWindowCollapsed(TRUE, ImGuiCond_Once);
	}
	// Create new window
	{
		ImGui::Begin("Control Panel", nullptr, ImGuiWindowFlags_NoMove); // ImGuiWindowFlags_NoResize

		// Framerate text
		FrameRateText(clientWidth, clientHeight);
		if (bRaytracingSupported) RaytraycingEnableCheckBox(pArgSet);
		// Lights
		LightHeader(pArgSet, lights, numLights, pendingLights);
		// Shading objects
		ShadingObjectHeader(pArgSet);

		ImGui::End(); 
	}

	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), pCmdList);

	return TRUE;
}