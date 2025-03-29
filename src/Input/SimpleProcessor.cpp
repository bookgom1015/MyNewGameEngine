#include "Input/SimpleInputProcessor.hpp"

using namespace Input;

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

LRESULT SimpleInputProcessor::MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	// WM_ACTIVATE is sent when the window is activated or deactivated.  
	// We pause the game when the window is deactivated and unpause it 
	// when it becomes active.
	case WM_ACTIVATE: {
		if (LOWORD(wParam) == WA_INACTIVE) {
			bAppPaused = TRUE;
		}
		else {
			bAppPaused = FALSE;
		}
		return 0;
	}
	// WM_SIZE is sent when the user resizes the window.  
	case WM_SIZE: {
		// Save the new client area dimensions.
		UINT width = LOWORD(lParam);
		UINT height = HIWORD(lParam);
		if (wParam == SIZE_MINIMIZED) {
			bAppPaused = TRUE;
			bMinimized = TRUE;
			bMaximized = FALSE;
		}
		else if (wParam == SIZE_MAXIMIZED) {
			bAppPaused = FALSE;
			bMinimized = FALSE;
			bMaximized = TRUE;
			if (bRegisteredOnResizeFunc) mOnResizeFunc(width, height);
		}
		else if (wParam == SIZE_RESTORED) {
			// Restoring from minimized state?
			if (bMinimized) {
				bAppPaused = FALSE;
				bMinimized = FALSE;
				if (bRegisteredOnResizeFunc) mOnResizeFunc(width, height);
			}

			// Restoring from maximized state?
			else if (bMaximized) {
				bAppPaused = FALSE;
				bMaximized = FALSE;
				if (bRegisteredOnResizeFunc) mOnResizeFunc(width, height);
			}
			// If user is dragging the resize bars, we do not resize 
			// the buffers here because as the user continuously 
			// drags the resize bars, a stream of WM_SIZE messages are
			// sent to the window, and it would be pointless (and slow)
			// to resize for each WM_SIZE message received from dragging
			// the resize bars.  So instead, we reset after the user is 
			// done resizing the window and releases the resize bars, which 
			// sends a WM_EXITSIZEMOVE message.
			else if (bResizing) {

			}
			// API call such as SetWindowPos or mSwapChain->SetFullscreenState.
			else {
				if (bRegisteredOnResizeFunc) mOnResizeFunc(width, height);
			}
		}
		return 0;
	}
				// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE: {
		bAppPaused = TRUE;
		bResizing = TRUE;
		return 0;
	}
	// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
	// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE: {
		bAppPaused = FALSE;
		bResizing = FALSE;

		RECT clientRect;
		GetClientRect(hWnd, &clientRect);

		UINT width = static_cast<UINT>(clientRect.right - clientRect.left);
		UINT height = static_cast<UINT>(clientRect.bottom - clientRect.top);

		if (bRegisteredOnResizeFunc) mOnResizeFunc(width, height);
		return 0;
	}
	// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY: {
		bDestroying = TRUE;
		PostQuitMessage(0);
		return 0;
	}
	 // The WM_MENUCHAR message is sent when a menu is active and the user presses 
	 // a key that does not correspond to any mnemonic or accelerator key. 
	case WM_MENUCHAR: {
		// Don't beep when we alt-enter.
		return MAKELRESULT(0, MNC_CLOSE);
	}
	// Catch this message so to prevent the window from becoming too small.
	case WM_GETMINMAXINFO: {
		return 0;
	}
	case WM_KEYUP: {
		OnKeyboardInput(WM_KEYUP, wParam, lParam);
		return 0;
	}
	case WM_KEYDOWN: {
		OnKeyboardInput(WM_KEYDOWN, wParam, lParam);
		return 0;
	}
	case WM_LBUTTONDOWN: {
		return 0;
	}
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

void SimpleInputProcessor::RegisterOnResizeFunc(const OnResizeFunc& func) {
	mOnResizeFunc = func;
	bRegisteredOnResizeFunc = TRUE;
}

void SimpleInputProcessor::OnKeyboardInput(UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (wParam) {
	case VK_ESCAPE: {
		if (msg == WM_KEYUP) {
			bDestroying = TRUE;
			PostQuitMessage(0);
		}
		return;
	}
	}
}