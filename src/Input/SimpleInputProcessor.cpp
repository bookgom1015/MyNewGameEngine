#include "Input/SimpleInputProcessor.hpp"

using namespace Input;
using namespace DirectX;
using namespace DirectX::SimpleMath;

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

bool SimpleInputProcessor::Initialize(Common::Debug::LogFile* const pLogFile) {
	CheckReturn(mpLogFile, InputProcessor::Initialize(pLogFile));

	return true;
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
		static_cast<float>(cursorPos.x) - static_cast<float>(wndRect.left),
		static_cast<float>(cursorPos.y) - static_cast<float>(wndRect.top));

	if (mInputState.Mouse.IsRelativeMouseMode()) {
		const auto centerX = static_cast<int>((wndRect.left + wndRect.right) * 0.5f);
		const auto centerY = static_cast<int>((wndRect.top + wndRect.bottom) * 0.5f);

		SetCursorPos(centerX, centerY);

		prevPos.x = static_cast<float>(centerX) - static_cast<float>(wndRect.left);
		prevPos.y = static_cast<float>(centerY) - static_cast<float>(wndRect.top);
	}

	Vector2 currPos = mInputState.Mouse.MousePosition();
	SetMouseDelta(currPos.x - prevPos.x, currPos.y - prevPos.y);

	ProcessInputIgnorance();
}