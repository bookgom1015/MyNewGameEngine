#pragma once

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
		BOOL mbIsWin32Initialized{};

		Common::Debug::LogFile* mpLogFile{};

		ImGuiContext* mpContext{};
	};
}