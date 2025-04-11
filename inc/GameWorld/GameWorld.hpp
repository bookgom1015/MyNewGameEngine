#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>

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
	}

	namespace Input {
		class InputProcessor;
	}
}

namespace GameWorld {
	namespace Foundation::Core {
		class ActorManager;
	}

	class GameWorldClass {
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
		BOOL InitActorManager();
		BOOL CreateRenderer();
		BOOL CreateInputProcessor();
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
		BOOL bInitialized = FALSE;

		Common::Debug::LogFile* mpLogFile = nullptr;

		// Multi-threading variables
		std::mutex mStageMutex;
		ProcessingStage mStage = ProcessingStage::E_DrawFinished;

		std::condition_variable mInputCV;
		std::condition_variable mUpdateCV;
		std::condition_variable mDrawCV;

		// Windows
		std::unique_ptr<Common::Foundation::Core::WindowsManager> mWindowsManager;

		// Actor manager
		std::unique_ptr<GameWorld::Foundation::Core::ActorManager> mActorManager;

		// Renderer
		std::unique_ptr<Common::Render::Renderer> mRenderer;

		// Timer
		std::unique_ptr<Common::Foundation::Core::GameTimer> mGameTimer;

		// Input processor
		std::unique_ptr<Common::Input::InputProcessor> mInputProcessor;
	};
}

#include "GameWorld.inl"