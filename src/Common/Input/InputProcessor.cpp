#include "Common/Input/InputProcessor.hpp"

using namespace Common::Input;
using namespace DirectX;

namespace {
	BOOL GetKeyButtonValue(INT key) {
		SHORT status = GetAsyncKeyState(key);
		if (status & 0x8000 || status & 0x8001) return TRUE;
		else return FALSE;
	}

	ButtonStates GetKeyButtonState(INT key) {
		SHORT status = GetAsyncKeyState(key);
		if (status & 0x0000) return ButtonStates::E_None;
		else if (status & 0x8000) return ButtonStates::E_Pressed;
		else if (status & 0x0001) return ButtonStates::E_Released;
		else return ButtonStates::E_Held;
	}
}

BOOL KeyboardState::KeyValue(INT key) const {
	return GetKeyButtonValue(key);
}

ButtonStates KeyboardState::KeyState(INT key) const {
	return GetKeyButtonState(key);
}

void MouseState::WheelUp() {
	mScrollWheelAccum += 1.f;
}

void MouseState::WheelDown() {
	mScrollWheelAccum -= 1.f;
}

BOOL MouseState::ButtonValue(INT button) const { 
	return GetKeyButtonValue(button);
}

ButtonStates MouseState::ButtonState(INT button) const { 
	return GetKeyButtonState(button); 
}

BOOL InputProcessor::Initialize(Common::Debug::LogFile* const pLogFile) {
	mpLogFile = pLogFile;

	return TRUE;
}

void InputProcessor::SetCursorVisibility(BOOL visible) {
	ShowCursor(visible);
}

void InputProcessor::SetMouseMode(MouseState::MouseModes mode) {
	mInputState.Mouse.mMouseMode = mode;
}

void InputProcessor::IgnoreMouseInput() {
	mInputState.Mouse.mbIsIgnored = TRUE;
}

void InputProcessor::SetMousePosition(FLOAT x, FLOAT y) {
	mInputState.Mouse.mMousePos.x = x;
	mInputState.Mouse.mMousePos.y = y;
}

void InputProcessor::SetMousePosition(DirectX::XMFLOAT2 pos) {
	mInputState.Mouse.mMousePos = pos;
}

void InputProcessor::SetMouseDelta(FLOAT dx, FLOAT dy) {
	mInputState.Mouse.mMouseDelta.x = dx;
	mInputState.Mouse.mMouseDelta.y = dy;
}

void InputProcessor::SetMouseDelta(DirectX::XMFLOAT2 delta) {
	mInputState.Mouse.mMouseDelta = delta;
}

void InputProcessor::ProcessInputIgnorance() {
	if (mInputState.Mouse.mbIsIgnored) {
		mInputState.Mouse.mbIsIgnored = FALSE;
		mInputState.Mouse.mMouseDelta = { 0.f,0.f };
	}
}