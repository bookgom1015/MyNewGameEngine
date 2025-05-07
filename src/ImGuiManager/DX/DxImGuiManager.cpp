#include "ImGuiManager/DX/DxImGuiManager.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Core/WindowsManager.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Foundation/Core/CommandObject.hpp"
#include "Render/DX/Foundation/Core/DescriptorHeap.hpp"
#include "Render/DX/Foundation/Core/SwapChain.hpp"
#include "Render/DX/Foundation/Resource/FrameResource.hpp"
#include "Render/DX/Foundation/Resource/GpuResource.hpp"

#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_dx12.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

using namespace ImGuiManager::DX;

extern "C" ImGuiManagerAPI Common::ImGuiManager::ImGuiManager* ImGuiManager::CreateImGuiManager() {
	return new DxImGuiManager();
}

extern "C" ImGuiManagerAPI void ImGuiManager::DestroyImGuiManager(Common::ImGuiManager::ImGuiManager* const imGuiManager) {
	delete imGuiManager;
}

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

void DxImGuiManager::CleanUpD3D12() {
	if (mbIsD3D12Initialized) ImGui_ImplDX12_Shutdown();
}

void DxImGuiManager::HookMsgCallback(Common::Foundation::Core::WindowsManager* const pWndManager) {
	pWndManager->HookMsgCallback(ImGui_ImplWin32_WndProcHandler);
}

BOOL DxImGuiManager::DrawImGui(
		ID3D12GraphicsCommandList6* const pCmdList,
		UINT clientWidth, UINT clientHeight) {
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// ControlPanel
	{
		ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiCond_Once);
		ImGui::SetNextWindowSize(ImVec2(340.f, static_cast<FLOAT>(clientHeight)), ImGuiCond_Always);
		ImGui::SetNextWindowCollapsed(TRUE, ImGuiCond_Once);
		ImGui::Begin("Control Panel", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

		// Framerate text
		{
			CHAR buffer[64];
			snprintf(buffer, sizeof(buffer), "%.1f FPS (%.3f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);

			const float TextWidth = ImGui::CalcTextSize(buffer).x;
			const float RegionWidth = ImGui::GetContentRegionAvail().x;

			ImGui::SetCursorPosX(RegionWidth - TextWidth);
			ImGui::TextUnformatted(buffer);
			ImGui::NewLine();
		}
	
		ImGui::End();
	}

	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), pCmdList);

	return TRUE;
}