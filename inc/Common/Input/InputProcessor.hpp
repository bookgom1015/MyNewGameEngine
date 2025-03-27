#pragma once

#include <functional>

#include <Windows.h>

#include "Common/Debug/Logger.hpp"

#ifdef _DLLEXPORT
	#ifndef InputProcessorAPI
		#define InputProcessorAPI __declspec(dllexport)
	#endif
#else
	#ifndef InputProcessorAPI
		#define InputProcessorAPI __declspec(dllimport)
	#endif
#endif

namespace Common::Input {
	class InputProcessor {
	public:
		using OnResizeFunc = std::function<void(UINT width, UINT height)>;
	public:
		InputProcessorAPI virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile);
		InputProcessorAPI virtual void CleanUp() = 0;

	public:
		InputProcessorAPI virtual LRESULT MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) = 0;
		InputProcessorAPI virtual void RegisterOnResizeFunc(const OnResizeFunc& func) = 0;
		InputProcessorAPI virtual void Halt();

	public:
		__forceinline constexpr BOOL AppPaused() const;
		__forceinline constexpr BOOL Minimized() const;
		__forceinline constexpr BOOL Maximized() const;
		__forceinline constexpr BOOL Resizing() const;
		__forceinline constexpr BOOL Fullscreen() const;
		__forceinline constexpr BOOL Destroying() const;


	protected:
		Common::Debug::LogFile* mpLogFile = nullptr;

		BOOL	  bAppPaused = FALSE;		// Is the application paused?
		BOOL	  bMinimized = FALSE;		// Is the application minimized?
		BOOL	  bMaximized = FALSE;		// Is the application maximized?
		BOOL	  bResizing = FALSE;		// Are the resize bars being dragged?
		BOOL	  bFullscreenState = FALSE;	// Fullscreen enabled 
		BOOL	  bDestroying = FALSE;
	};
}

constexpr BOOL Common::Input::InputProcessor::AppPaused() const {
	return bAppPaused;
}

constexpr BOOL Common::Input::InputProcessor::Minimized() const {
	return bMinimized;
}

constexpr BOOL Common::Input::InputProcessor::Maximized() const {
	return bMaximized;
}

constexpr BOOL Common::Input::InputProcessor::Resizing() const {
	return bResizing;
}

constexpr BOOL Common::Input::InputProcessor::Fullscreen() const {
	return bFullscreenState;
}

constexpr BOOL Common::Input::InputProcessor::Destroying() const {
	return bDestroying;
}