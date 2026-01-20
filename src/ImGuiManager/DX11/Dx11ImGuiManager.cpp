#include "ImGuiManager/DX11/pch_imgui_dx11.h"
#include "ImGuiManager/DX11/Dx11ImGuiManager.hpp"

using namespace ImGuiManager::DX11;

extern "C" ImGuiManagerAPI Common::ImGuiManager::ImGuiManager* ImGuiManager::CreateImGuiManager() {
	return new Dx11ImGuiManager();
}

extern "C" ImGuiManagerAPI void ImGuiManager::DestroyImGuiManager(Common::ImGuiManager::ImGuiManager* const imGuiManager) {
	delete imGuiManager;
}

Dx11ImGuiManager::Dx11ImGuiManager() {}

Dx11ImGuiManager::~Dx11ImGuiManager() {}

BOOL Dx11ImGuiManager::InitializeD3D11() {
	mbIsD3D11Initialized = TRUE;

	return TRUE;
}

void Dx11ImGuiManager::CleanUpD3D11() {
	if (mbIsD3D11Initialized) {}
}