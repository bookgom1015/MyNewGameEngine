#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <Windows.h>

#include <SimpleMath.h>

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
		InputProcessorAPI virtual bool KeyValue(int key) const;
		InputProcessorAPI virtual ButtonStates KeyState(int key) const;
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
		__forceinline DirectX::SimpleMath::Vector2 MousePosition() const;
		__forceinline DirectX::SimpleMath::Vector2 MouseDelta() const;
		__forceinline float ScrollWheel() const;
		__forceinline bool IsInputIgnored() const;
		__forceinline bool IsRelativeMouseMode() const;

	public:
		InputProcessorAPI virtual void WheelUp();
		InputProcessorAPI virtual void WheelDown();

		InputProcessorAPI virtual bool ButtonValue(int button) const;
		InputProcessorAPI virtual ButtonStates ButtonState(int button) const;

	private:
		DirectX::SimpleMath::Vector2 mMousePos{ 0.f, 0.0f };
		DirectX::SimpleMath::Vector2 mMouseDelta{ 0.f, 0.0f };

		float mScrollWheel{};
		float mScrollWheelAccum{};

		bool mbIsIgnored{ true };

		MouseModes mMouseMode{ MouseModes::E_Absolute };
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
		InputProcessorAPI virtual bool Initialize(Common::Debug::LogFile* const pLogFile);
		InputProcessorAPI virtual void CleanUp() = 0;
				
	public:
		InputProcessorAPI virtual void SetCursorVisibility(bool visible);
		InputProcessorAPI virtual void SetMouseMode(MouseState::MouseModes mode);
		InputProcessorAPI virtual void IgnoreMouseInput();

		InputProcessorAPI virtual void OnKeyboardInput(UINT msg, WPARAM wParam, LPARAM lParam) = 0;
		InputProcessorAPI virtual void OnMouseInput(HWND hWnd) = 0;

	public:
		__forceinline InputState GetInputState() const;

	protected:
		void SetMousePosition(float x, float y);
		void SetMousePosition(const DirectX::SimpleMath::Vector2& pos);

		void SetMouseDelta(float dx, float dy);
		void SetMouseDelta(const DirectX::SimpleMath::Vector2& delta);

		void ProcessInputIgnorance();

	protected:
		Common::Debug::LogFile* mpLogFile{};

		InputState mInputState{};
	};
}

#include "Common/Input/InputProcessor.inl"