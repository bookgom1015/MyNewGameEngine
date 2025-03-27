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
		class GameTimer;
	}

	namespace Render {
		class Renderer;
	}

	namespace Input {
		class InputProcessor;
	}
}

class GameWorld {
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
	GameWorld();
	virtual ~GameWorld();

public:
	BOOL Initialize(Common::Debug::LogFile* const pLogFile);
	BOOL RunLoop();
	void CleanUp();

private: // Functions that is called only once
	BOOL CreateMainWindow();
	BOOL GetHWInfo();
	BOOL CreateRenderer();
	BOOL CreateInputProcessor();

private: // Functions that is called whenever a message is called
	void OnResize(UINT width, UINT height);

private: // Main loop stage functions
	BOOL ProcessInput();
	BOOL Update();
	BOOL Draw();

public:
	static GameWorld* spGameWorld;

private:
	// Variables which is associated with main window
	HINSTANCE mhInst			= NULL;		// Application instance handle
	HWND	  mhMainWnd			= NULL;		// Main window handle
	BOOL	  bInitialized		= FALSE;

	Common::Debug::LogFile* mpLogFile = nullptr;

	// Multi-threading variables
	std::mutex mStageMutex;
	ProcessingStage mStage = ProcessingStage::E_DrawFinished;

	std::condition_variable mInputCV;
	std::condition_variable mUpdateCV;
	std::condition_variable mDrawCV;

	// Renderer
	std::unique_ptr<Common::Render::Renderer> mRenderer;

	// Timer
	std::unique_ptr<Common::Foundation::Core::GameTimer> mGameTimer;

public:
	// Input processor
	std::unique_ptr<Common::Input::InputProcessor> mInputProcessor;
};