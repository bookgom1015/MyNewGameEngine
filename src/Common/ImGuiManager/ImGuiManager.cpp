#include "Common/ImGuiManager/pch_imgui_common.h"
#include "Common/ImGuiManager/ImGuiManager.hpp"
#include "Common/Debug/Logger.hpp"

using namespace Common::ImGuiManager;

BOOL ImGuiManager::Initialize(Common::Debug::LogFile* const pLogFile, HWND hWnd) {
	mpLogFile = pLogFile;

	// Setup dear ImGui context.
	IMGUI_CHECKVERSION();
	mpContext = ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.Fonts->AddFontDefault();
	io.Fonts->Build();

	// Setup Dear ImGui style.
	ImGui::StyleColorsDark();

	// Setup platform/renderer backends
	CheckReturn(mpLogFile, ImGui_ImplWin32_Init(hWnd));

	mbIsWin32Initialized = TRUE;

	return TRUE;
}

void ImGuiManager::CleanUp() {
	if (mbIsWin32Initialized) ImGui_ImplWin32_Shutdown();
	if (mpContext != nullptr) ImGui::DestroyContext(mpContext);
}