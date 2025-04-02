#pragma once

#include <functional>

#include <Windows.h>

#include <DirectXMath.h>

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
	enum ButtonStates {
		E_None,
		E_Pressed,
		E_Released,
		E_Held
	};

	class KeyboardState {
	private:
		friend class InputProcessor;

	public:
		KeyboardState() = default;
		virtual ~KeyboardState() = default;

	public:
		BOOL KeyValue(INT key) const;
		ButtonStates KeyState(INT key) const;
	};

	class MouseState {
	private:
		friend class InputProcessor;

	public:
		enum MouseModes {
			E_Absolute,
			E_Relative,
		};

	public:
		MouseState() = default;
		virtual ~MouseState() = default;

	public:
		__forceinline DirectX::XMFLOAT2 MousePosition() const;
		__forceinline DirectX::XMFLOAT2 MouseDelta() const;
		__forceinline FLOAT ScrollWheel() const;
		__forceinline BOOL IsInputIgnored() const;
		__forceinline BOOL IsRelativeMouseMode() const;

	public:
		void WheelUp();
		void WheelDown();

		BOOL ButtonValue(INT button) const;
		ButtonStates ButtonState(INT button) const;

	private:
		DirectX::XMFLOAT2 mMousePos = { 0.f, 0.0f };
		DirectX::XMFLOAT2 mMouseDelta = { 0.f, 0.0f };

		FLOAT mScrollWheel = 0.f;
		FLOAT mScrollWheelAccum = 0.f;

		BOOL mbIsIgnored = TRUE;

		MouseModes mMouseMode = MouseModes::E_Absolute;
	};

	class ControllerState {
	private:
		friend class InputProcessor;

	public:
		ControllerState() = default;
		virtual ~ControllerState() = default;
	};

	struct InputState {
		KeyboardState Keyboard;
		MouseState Mouse;
		ControllerState Controller;
	};

	class InputProcessor {
	public:
		using OnResizeFunc = std::function<void(UINT width, UINT height)>;

	public:
		__forceinline constexpr BOOL AppPaused() const;
		__forceinline constexpr BOOL Minimized() const;
		__forceinline constexpr BOOL Maximized() const;
		__forceinline constexpr BOOL Resizing() const;
		__forceinline constexpr BOOL Fullscreen() const;
		__forceinline constexpr BOOL Destroying() const;

	public:
		InputProcessorAPI virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile);
		InputProcessorAPI virtual void CleanUp() = 0;

	public:
		InputProcessorAPI virtual LRESULT MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) = 0;

		InputProcessorAPI virtual void RegisterOnResizeFunc(const OnResizeFunc& func) = 0;
		InputProcessorAPI virtual void Halt();

	public:
		InputProcessorAPI void virtual SetCursorVisibility(BOOL visible);
		InputProcessorAPI void virtual SetMouseMode(MouseState::MouseModes mode);
		InputProcessorAPI void virtual IgnoreMouseInput();

	protected:
		void SetMousePosition(FLOAT x, FLOAT y);
		void SetMousePosition(DirectX::XMFLOAT2 pos);

		void SetMouseDelta(FLOAT dx, FLOAT dy);
		void SetMouseDelta(DirectX::XMFLOAT2 delta);

		void ProcessInputIgnorance();

	protected:
		Common::Debug::LogFile* mpLogFile = nullptr;

		BOOL	  bAppPaused	   = FALSE; // Is the application paused?
		BOOL	  bMinimized	   = FALSE; // Is the application minimized?
		BOOL	  bMaximized	   = FALSE; // Is the application maximized?
		BOOL	  bResizing		   = FALSE; // Are the resize bars being dragged?
		BOOL	  bFullscreenState = FALSE; // Fullscreen enabled 
		BOOL	  bDestroying	   = FALSE;

		InputState mInputState;
	};
}

#include "Common/Input/InputProcessor.inl"