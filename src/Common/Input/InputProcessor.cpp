#include "Common/Input/InputProcessor.hpp"

using namespace Common::Input;
using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace {
	bool GetKeyButtonValue(int key) {
		SHORT status = GetAsyncKeyState(key);
		if (status & 0x8000 || status & 0x8001) return true;
		else return false;
	}

	ButtonStates GetKeyButtonState(int key) {
		SHORT status = GetAsyncKeyState(key);
		if (status & 0x0000) return ButtonStates::E_None;
		else if (status & 0x8000) return ButtonStates::E_Pressed;
		else if (status & 0x0001) return ButtonStates::E_Released;
		else return ButtonStates::E_Held;
	}
}

bool KeyboardState::KeyValue(int key) const {
	return GetKeyButtonValue(key);
}

ButtonStates KeyboardState::KeyState(int key) const {
	return GetKeyButtonState(key);
}

void MouseState::WheelUp() {
	mScrollWheelAccum += 1.f;
}

void MouseState::WheelDown() {
	mScrollWheelAccum -= 1.f;
}

bool MouseState::ButtonValue(int button) const { 
	return GetKeyButtonValue(button);
}

ButtonStates MouseState::ButtonState(int button) const { 
	return GetKeyButtonState(button); 
}

bool InputProcessor::Initialize(Common::Debug::LogFile* const pLogFile) {
	mpLogFile = pLogFile;

	return true;
}

void InputProcessor::SetCursorVisibility(bool visible) {
	ShowCursor(visible);
}

void InputProcessor::SetMouseMode(MouseState::MouseModes mode) {
	mInputState.Mouse.mMouseMode = mode;
}

void InputProcessor::IgnoreMouseInput() {
	mInputState.Mouse.mbIsIgnored = true;
}

void InputProcessor::SetMousePosition(float x, float y) {
	mInputState.Mouse.mMousePos.x = x;
	mInputState.Mouse.mMousePos.y = y;
}

void InputProcessor::SetMousePosition(const Vector2& pos) {
	mInputState.Mouse.mMousePos = pos;
}

void InputProcessor::SetMouseDelta(float dx, float dy) {
	mInputState.Mouse.mMouseDelta.x = dx;
	mInputState.Mouse.mMouseDelta.y = dy;
}

void InputProcessor::SetMouseDelta(const Vector2& delta) {
	mInputState.Mouse.mMouseDelta = delta;
}

void InputProcessor::ProcessInputIgnorance() {
	if (mInputState.Mouse.mbIsIgnored) {
		mInputState.Mouse.mbIsIgnored = false;
		mInputState.Mouse.mMouseDelta = { 0.f,0.f };
	}
}