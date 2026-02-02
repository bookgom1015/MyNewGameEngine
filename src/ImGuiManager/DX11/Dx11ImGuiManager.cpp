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
	ImGui::SetCurrentContext(mpContext);

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
		//if (bRaytracingSupported) RaytraycingEnableCheckBox(pArgSet);
		// Lights
		LightHeader(pArgSet, lights, numLights, pendingLights);
		// Shading objects
		ShadingObjectHeader(pArgSet);

		ImGui::End();
	}

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	return TRUE;
}