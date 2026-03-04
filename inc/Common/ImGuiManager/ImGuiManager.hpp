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

#include <imgui/imgui.h>

struct ImGuiContext;

namespace Common::Debug {
	struct LogFile;
}

namespace Common::Foundation::Core {
	class WindowsManager;
}

namespace Common {
	namespace Foundation {
		struct Light;
	}

	namespace Render::ShadingArgument {
		struct ShadingArgumentSet;
	}
}

namespace Common::ImGuiManager {
	namespace LogLevel {
		enum Type {
			E_Info = 0,
			E_Warning,
			E_Error,
			E_Critical,
			Count
		};
	}

	struct LogEntry {
		LogLevel::Type Level;
		std::string Message;
	};

	using DisplayTexture = std::map<std::string, ImTextureID>;

	class ImGuiManager {
	public:
		ImGuiManager() = default;
		virtual ~ImGuiManager() = default;

	public:
		ImGuiManagerAPI virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, HWND hWnd);
		ImGuiManagerAPI virtual void CleanUp();

		ImGuiManagerAPI void HookMsgCallback(
			Common::Foundation::Core::WindowsManager* const pWndManager);

		ImGuiManagerAPI void AddDisplayTexture(const std::string& name, ImTextureID id);

	protected:
		ImGuiManagerAPI void LightHeader(
			Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet,
			Common::Foundation::Light* lights[],
			UINT numLights,
			std::queue<std::shared_ptr<Common::Foundation::Light>>& pendingLights);

		ImGuiManagerAPI virtual void MarginalSpacing();
		ImGuiManagerAPI virtual void TextWithBg(const char* pTxt);

		ImGuiManagerAPI virtual void MenuBar(Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet);
		ImGuiManagerAPI virtual void Scene() = 0;
		ImGuiManagerAPI virtual void Texture();
		ImGuiManagerAPI virtual void Inspector();
		ImGuiManagerAPI virtual void Outliner();
		ImGuiManagerAPI virtual void Content();
		ImGuiManagerAPI virtual void Profiler();
		ImGuiManagerAPI virtual void LogUI();

	protected:
		BOOL mbIsWin32Initialized{};

		Common::Debug::LogFile* mpLogFile{};
		HWND mhMainWnd{};

		ImGuiContext* mpContext{};

		FLOAT mFrameTimes[3000]{};
		UINT mFrameOffset{};

		bool mbSceneOpened{ true };
		bool mbTextureOpened{ true };
		bool mbInspectorOpened{ true };
		bool mbOutlinerOpened{ true };
		bool mbContentOpened{ true };
		bool mbProfilerOpened{ false };
		bool mbLogOpened{ true };

		DisplayTexture mDisplayTextures{};
		std::string mSelectedTexture{};

		bool mbAutoScroll{ true };
		bool mbScrollToBottom{ false };

		std::vector<LogEntry> mLogs{};
	};
}