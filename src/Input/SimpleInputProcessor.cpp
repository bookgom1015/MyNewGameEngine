#include "Input/SimpleInputProcessor.hpp"

using namespace Input;
using namespace DirectX;

extern "C" InputProcessorAPI Common::Input::InputProcessor* Input::CreateInputProcessor() {
	return new SimpleInputProcessor();
}

extern "C" InputProcessorAPI void Input::DestroyInputProcessor(Common::Input::InputProcessor* const inputProcessor) {
	delete inputProcessor;
}

SimpleInputProcessor::SimpleInputProcessor() {}

SimpleInputProcessor::~SimpleInputProcessor() {
	CleanUp();
}

BOOL SimpleInputProcessor::Initialize(Common::Debug::LogFile* const pLogFile) {
	CheckReturn(mpLogFile, InputProcessor::Initialize(pLogFile));

	return TRUE;
}

void SimpleInputProcessor::CleanUp() {}

void SimpleInputProcessor::OnKeyboardInput(UINT msg, WPARAM wParam, LPARAM lParam) {
	
}

void SimpleInputProcessor::OnMouseInput(HWND hWnd) {
	XMFLOAT2 prevPos = mInputState.Mouse.MousePosition();

	RECT wndRect;
	GetWindowRect(hWnd, &wndRect);
	
	POINT cursorPos;
	GetCursorPos(&cursorPos);
	
	SetMousePosition(
		static_cast<FLOAT>(cursorPos.x) - static_cast<FLOAT>(wndRect.left),
		static_cast<FLOAT>(cursorPos.y) - static_cast<FLOAT>(wndRect.top));

	if (mInputState.Mouse.IsRelativeMouseMode()) {
		const auto centerX = static_cast<INT>((wndRect.left + wndRect.right) * 0.5f);
		const auto centerY = static_cast<INT>((wndRect.top + wndRect.bottom) * 0.5f);

		SetCursorPos(centerX, centerY);

		prevPos.x = static_cast<FLOAT>(centerX) - static_cast<FLOAT>(wndRect.left);
		prevPos.y = static_cast<FLOAT>(centerY) - static_cast<FLOAT>(wndRect.top);
	}

	XMFLOAT2 currPos = mInputState.Mouse.MousePosition();
	SetMouseDelta(currPos.x - prevPos.x, currPos.y - prevPos.y);

	ProcessInputIgnorance();
}