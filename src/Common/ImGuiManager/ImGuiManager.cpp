#include "Common/ImGuiManager/ImGuiManager.hpp"
#include "Common/Debug/Logger.hpp"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_win32.h>

using namespace Common::ImGuiManager;

BOOL ImGuiManager::Initialize(Common::Debug::LogFile* const pLogFile, HWND hWnd) {
	mpLogFile = pLogFile;

	// Setup dear ImGui context.
	IMGUI_CHECKVERSION();
	mpContext = ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO(); (void)io;

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