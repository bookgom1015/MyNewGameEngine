#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <Windows.h>

#include <DirectXMath.h>

#include "Common/Debug/Logger.hpp"
#include "Input/KeyCodes.hpp"

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
		InputProcessorAPI virtual BOOL KeyValue(INT key) const;
		InputProcessorAPI virtual ButtonStates KeyState(INT key) const;
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
		InputProcessorAPI virtual void WheelUp();
		InputProcessorAPI virtual void WheelDown();

		InputProcessorAPI virtual BOOL ButtonValue(INT button) const;
		InputProcessorAPI virtual ButtonStates ButtonState(INT button) const;

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
		InputProcessorAPI virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile);
		InputProcessorAPI virtual void CleanUp() = 0;
				
	public:
		InputProcessorAPI virtual void SetCursorVisibility(BOOL visible);
		InputProcessorAPI virtual void SetMouseMode(MouseState::MouseModes mode);
		InputProcessorAPI virtual void IgnoreMouseInput();

		InputProcessorAPI virtual void OnKeyboardInput(UINT msg, WPARAM wParam, LPARAM lParam) = 0;
		InputProcessorAPI virtual void OnMouseInput(HWND hWnd) = 0;

	public:
		__forceinline InputState GetInputState() const;

	protected:
		void SetMousePosition(FLOAT x, FLOAT y);
		void SetMousePosition(DirectX::XMFLOAT2 pos);

		void SetMouseDelta(FLOAT dx, FLOAT dy);
		void SetMouseDelta(DirectX::XMFLOAT2 delta);

		void ProcessInputIgnorance();

	protected:
		Common::Debug::LogFile* mpLogFile = nullptr;

		InputState mInputState;
	};
}

#include "Common/Input/InputProcessor.inl"