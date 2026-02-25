#include "ImGuiManager/DX11/pch_imgui_dx11.h"
#include "ImGuiManager/DX11/Dx11ImGuiManager.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Core/WindowsManager.hpp"
#include "Render/DX11/Foundation/Core/Device.hpp"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

using namespace ImGuiManager::DX11;

extern "C" ImGuiManagerAPI Common::ImGuiManager::ImGuiManager* ImGuiManager::CreateImGuiManager() {
	return new Dx11ImGuiManager();
}

extern "C" ImGuiManagerAPI void ImGuiManager::DestroyImGuiManager(Common::ImGuiManager::ImGuiManager* const imGuiManager) {
	delete imGuiManager;
}

Dx11ImGuiManager::Dx11ImGuiManager() {}

Dx11ImGuiManager::~Dx11ImGuiManager() {}

BOOL Dx11ImGuiManager::InitializeD3D11(
		Render::DX11::Foundation::Core::Device* const pDevice) {
	CheckReturn(mpLogFile, ImGui_ImplDX11_Init(
		pDevice->mDevice.Get(), pDevice->mContext.Get()));

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.Fonts->AddFontDefault();
	io.Fonts->Build();

	mbIsD3D11Initialized = TRUE;

	return TRUE;
}

void Dx11ImGuiManager::CleanUp() {
	if (mbIsD3D11Initialized) ImGui_ImplDX11_Shutdown();
	ImGuiManager::CleanUp();
}

BOOL Dx11ImGuiManager::DrawImGui(
		Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet,
		Common::Foundation::Light* lights[],
		UINT numLights,
		std::queue<std::shared_ptr<Common::Foundation::Light>>& pendingLights,
		UINT clientWidth, UINT clientHeight) {
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::DockSpaceOverViewport(
		0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

	// Create new window
	{
		//ImGui::Begin("Control Panel");
		//
		//// Framerate text
		//FrameRateText(clientWidth, clientHeight);
		////if (bRaytracingSupported) RaytraycingEnableCheckBox(pArgSet);
		//// Lights
		//LightHeader(pArgSet, lights, numLights, pendingLights);
		//// Shading objects
		//ShadingObjectHeader(pArgSet);
		//
		//ImGuiContext* ctx = ImGui::GetCurrentContext();
		//ImGuiIO& io = ImGui::GetIO();
		//ImGui::Text("ctx=%p backend=%p", ctx, io.BackendPlatformUserData);
		//
		//ImGui::End();

		ImGui::Begin("Scene");
		ImGui::End();

		ImGui::Begin("Inspector");
		ImGui::End();
	}

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// Update and Render additional Platform Windows
	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

	return TRUE;
}