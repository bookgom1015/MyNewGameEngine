#pragma once

#include <condition_variable>
#include <mutex>
#include <Windows.h>

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
	BOOL Initialize();
	BOOL RunLoop();
	void CleanUp();

	LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private: // Functions that called only once
	BOOL CreateMainWindow();

private: // Functions that called whenever a message is called
	void OnResize(UINT width, UINT height);

private: // Main loop stage functions
	BOOL ProcessInput();
	BOOL Update();
	BOOL Draw();

public:
	static GameWorld* sGameWorld;

private:
	// Variables which is associated with main window
	HINSTANCE mhInst			= NULL;		// Application instance handle
	HWND	  mhMainWnd			= NULL;		// Main window handle
	BOOL	  bAppPaused		= FALSE;	// Is the application paused?
	BOOL	  bMinimized		= FALSE;	// Is the application minimized?
	BOOL	  bMaximized		= FALSE;	// Is the application maximized?
	BOOL	  bResizing			= FALSE;	// Are the resize bars being dragged?
	BOOL	  bFullscreenState  = FALSE;	// Fullscreen enabled 
	BOOL	  bDestroying		= FALSE;

	// Multi-threading variables
	std::mutex mStageMutex;
	ProcessingStage mStage = ProcessingStage::E_DrawFinished;

	std::condition_variable mInputCV;
	std::condition_variable mUpdateCV;
	std::condition_variable mDrawCV;
};