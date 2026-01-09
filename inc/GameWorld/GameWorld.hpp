#pragma once

#pragma comment(lib, "Renderer.lib")
#pragma comment(lib, "InputProcessor.lib")
#pragma comment(lib, "ImGuiManager.lib")

#include <condition_variable>
#include <memory>
#include <mutex>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <wrl.h>
#include <Windows.h>

namespace Common {
	namespace Debug {
		struct LogFile;
	}

	namespace Foundation::Core {
		class WindowsManager;
		class GameTimer;
	}

	namespace Render {
		class Renderer;

		namespace ShadingArgument {
			struct ShadingArgumentSet;
		}
	}

	namespace Input {
		class InputProcessor;
	}

	namespace ImGuiManager {
		class ImGuiManager;
	}
}

namespace GameWorld {
	namespace Foundation::Core {
		class ActorManager;
	}

	class GameWorldClass {
		using ImGuiManagerDeleter = void(*)(Common::ImGuiManager::ImGuiManager*);
		using RendererDeleter = void(*)(Common::Render::Renderer*);
		using InputProcessorDeleter = void(*)(Common::Input::InputProcessor*);

	private:
		enum ProcessingStage {
			E_InputReady,
			E_InputFinished,
			E_UpdateReady,
			E_UpdateFinished,
			E_DrawReady,
			E_DrawFinished
		};

	public:
		GameWorldClass();
		virtual ~GameWorldClass();

	public:
		__forceinline Common::Foundation::Core::WindowsManager* WindowsManager() const;
		__forceinline Foundation::Core::ActorManager* ActorManager() const;
		__forceinline Common::Input::InputProcessor* InputProcessor() const;
		__forceinline Common::Render::Renderer* Renderer() const;

	public:
		BOOL Initialize(Common::Debug::LogFile* const pLogFile, HINSTANCE hInstance);
		BOOL RunLoop();
		void CleanUp();

	private: // Functions that is called only once
		BOOL BuildHWInfo();
		BOOL InitWindowsManager(HINSTANCE hInstance);
		BOOL CreateImGuiManager();
		BOOL CreateRenderer();
		BOOL CreateInputProcessor();
		BOOL InitActorManager();
		BOOL BuildScene();

	private: // Functions that is called whenever a message is called
		void OnResize(UINT width, UINT height);

	private: // Main loop stage functions
		BOOL ProcessInput();
		BOOL Update();
		BOOL Draw();

	public:
		static GameWorldClass* spGameWorld;

	private:
		BOOL bInitialized{};

		Common::Debug::LogFile* mpLogFile{};

		// Multi-threading variables
		std::mutex mStageMutex{};
		ProcessingStage mStage = ProcessingStage::E_DrawFinished;

		std::condition_variable mInputCV{};
		std::condition_variable mUpdateCV{};
		std::condition_variable mDrawCV{};

		// Windows
		std::unique_ptr<Common::Foundation::Core::WindowsManager> mWindowsManager{};

		// Actor manager
		std::unique_ptr<GameWorld::Foundation::Core::ActorManager> mActorManager{};

		// Renderer
		std::unique_ptr<Common::Render::Renderer, RendererDeleter> mRenderer{ nullptr, nullptr };
		std::unique_ptr<Common::Render::ShadingArgument::ShadingArgumentSet> mArgumentSet{};

		// Timer
		std::unique_ptr<Common::Foundation::Core::GameTimer> mGameTimer{};

		// Input processor
		std::unique_ptr<Common::Input::InputProcessor, InputProcessorDeleter> mInputProcessor{ nullptr, nullptr };

		// ImGui manager
		std::unique_ptr<Common::ImGuiManager::ImGuiManager, ImGuiManagerDeleter> mImGuiManager{ nullptr, nullptr };
	};
}

#include "GameWorld.inl"