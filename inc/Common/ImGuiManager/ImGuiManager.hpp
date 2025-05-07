#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <wrl.h>
#include <Windows.h>

#ifdef _DLLEXPORT
	#ifndef ImGuiManagerAPI
	#define ImGuiManagerAPI __declspec(dllexport)
	#endif
#else
	#ifndef ImGuiManagerAPI
	#define ImGuiManagerAPI __declspec(dllimport)
	#endif
#endif

struct ImGuiContext;

namespace Common::Debug {
	struct LogFile;
}

namespace Common::ImGuiManager {
	class ImGuiManager {
	public:
		ImGuiManager() = default;
		virtual ~ImGuiManager() = default;

	public:
		ImGuiManagerAPI virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, HWND hWnd);
		ImGuiManagerAPI virtual void CleanUp();

	protected:
		BOOL mbIsWin32Initialized = FALSE;

		Common::Debug::LogFile* mpLogFile = nullptr;

		ImGuiContext* mpContext = nullptr;
	};
}