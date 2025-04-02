#include "Common/Foundation/Core/WindowsManager.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Input/InputProcessor.hpp"

using namespace Common::Foundation::Core;

namespace {
	LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		// Forward hwnd on because we can get messages (e.g., WM_CREATE)
		// before CreateWindow returns, and thus before mhMainWnd is valid
		return WindowsManager::sWindowsManager->MsgProc(hwnd, msg, wParam, lParam);
	}

	INT_PTR CALLBACK DialogProc(HWND hDialog, UINT msg, WPARAM wParam, LPARAM lParam) {
		switch (msg) {
		case WM_INITDIALOG: {
			return TRUE;
		}
		case WM_CLOSE:
			EndDialog(hDialog, 0);
			break;
		}
		return FALSE;
	}
}

WindowsManager::SelectDialogInitDataPtr WindowsManager::MakeSelectDialogInitData() {
	return std::unique_ptr<SelectDialogInitData>(new SelectDialogInitData);
}

WindowsManager* WindowsManager::sWindowsManager = nullptr;

WindowsManager::WindowsManager() {
	sWindowsManager = this;
}

LRESULT CALLBACK WindowsManager::MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
		// WM_ACTIVATE is sent when the window is activated or deactivated.  
		// We pause the game when the window is deactivated and unpause it 
		// when it becomes active.
	case WM_ACTIVATE: {
		if (LOWORD(wParam) == WA_INACTIVE) {
			mbAppPaused = TRUE;
		}
		else {
			mbAppPaused = FALSE;
		}
		return 0;
	}
					// WM_SIZE is sent when the user resizes the window.  
	case WM_SIZE: {
		// Save the new client area dimensions.
		UINT width = LOWORD(lParam);
		UINT height = HIWORD(lParam);
		if (wParam == SIZE_MINIMIZED) {
			mbAppPaused = TRUE;
			mbMinimized = TRUE;
			mbMaximized = FALSE;
		}
		else if (wParam == SIZE_MAXIMIZED) {
			mbAppPaused = FALSE;
			mbMinimized = FALSE;
			mbMaximized = TRUE;
			if (mbRegisteredOnResizeFunc) mOnResizeFunc(width, height);
		}
		else if (wParam == SIZE_RESTORED) {
			// Restoring from minimized state?
			if (mbMinimized) {
				mbAppPaused = FALSE;
				mbMinimized = FALSE;
				if (mbRegisteredOnResizeFunc) mOnResizeFunc(width, height);
			}

			// Restoring from maximized state?
			else if (mbMaximized) {
				mbAppPaused = FALSE;
				mbMaximized = FALSE;
				if (mbRegisteredOnResizeFunc) mOnResizeFunc(width, height);
			}
			// If user is dragging the resize bars, we do not resize 
			// the buffers here because as the user continuously 
			// drags the resize bars, a stream of WM_SIZE messages are
			// sent to the window, and it would be pointless (and slow)
			// to resize for each WM_SIZE message received from dragging
			// the resize bars.  So instead, we reset after the user is 
			// done resizing the window and releases the resize bars, which 
			// sends a WM_EXITSIZEMOVE message.
			else if (mbResizing) {

			}
			// API call such as SetWindowPos or mSwapChain->SetFullscreenState.
			else {
				if (mbRegisteredOnResizeFunc) mOnResizeFunc(width, height);
			}
		}
		return 0;
	}
				// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE: {
		mbAppPaused = TRUE;
		mbResizing = TRUE;
		return 0;
	}
						 // WM_EXITSIZEMOVE is sent when the user releases the resize bars.
						 // Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE: {
		mbAppPaused = FALSE;
		mbResizing = FALSE;

		RECT clientRect;
		GetClientRect(hWnd, &clientRect);

		UINT width = static_cast<UINT>(clientRect.right - clientRect.left);
		UINT height = static_cast<UINT>(clientRect.bottom - clientRect.top);

		if (mbRegisteredOnResizeFunc) mOnResizeFunc(width, height);
		return 0;
	}
						// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY: {
		mbDestroyed = TRUE;
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
	case WM_KEYUP: 
	case WM_KEYDOWN: {
		if (msg == WM_KEYUP && wParam == VK_ESCAPE) {
			mbDestroyed = TRUE;
			PostQuitMessage(0);
		}
		else if (mbInputProcessorRegistered) mpInputProcessor->OnKeyboardInput(msg, wParam, lParam);
		return 0;
	}
	case WM_LBUTTONDOWN: {
		return 0;
	}
	case WM_MOUSEMOVE: {
		if (mbInputProcessorRegistered) mpInputProcessor->OnMouseInput(hWnd);
		return 0;
	}
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

BOOL WindowsManager::Initialize(Common::Debug::LogFile* const pLogFile, HINSTANCE hInstance, UINT wndWidth, UINT wndHeight) {
	mpLogFile = pLogFile;
	mhInst = hInstance;

	CheckReturn(mpLogFile, InitMainWnd(wndWidth, wndHeight));

	return TRUE;
}

void WindowsManager::RegisterOnResizeFunc(const OnResizeFunc& func) {
	mOnResizeFunc = func;
	mbRegisteredOnResizeFunc = TRUE;
}

void WindowsManager::DestroyWindow() {
	mbDestroyed = TRUE;
	PostQuitMessage(0);
}

void WindowsManager::RegisterInputProcessor(Common::Input::InputProcessor* const pInputProc) {
	mpInputProcessor = pInputProc;
	mbInputProcessorRegistered = TRUE;
}

BOOL WindowsManager::SelectDialog(SelectDialogInitData* const pInitData) {
	DLGTEMPLATE* dialogTemplate;

	BYTE dlgBuffer[1024] = { 0 };
	dialogTemplate = (DLGTEMPLATE*)dlgBuffer;

	dialogTemplate->style = WS_POPUP | WS_BORDER | WS_SYSMENU | DS_MODALFRAME | WS_CAPTION;
	dialogTemplate->dwExtendedStyle = 0;
	dialogTemplate->cdit = 0;
	dialogTemplate->x = 10;
	dialogTemplate->y = 10;
	dialogTemplate->cx = 200;
	dialogTemplate->cy = 150;

	DialogBoxIndirectParamW(mhInst, dialogTemplate, mhMainWnd, DialogProc, reinterpret_cast<LPARAM>(pInitData));

	return TRUE;
}

BOOL WindowsManager::InitMainWnd(UINT wndWidth, UINT wndHeight) {
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = mhInst;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(DKGRAY_BRUSH));
	wc.lpszMenuName = 0;
	wc.lpszClassName = L"MyNewGameEngine";
	if (!RegisterClassW(&wc)) ReturnFalse(mpLogFile, L"Failed to register the window class");

	// Compute window rectangle dimensions based on requested client area dimensions.
	RECT R = { 0, 0, static_cast<LONG>(wndWidth), static_cast<LONG>(wndHeight) };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, FALSE);
	INT width = R.right - R.left;
	INT height = R.bottom - R.top;

	INT outputWidth = GetSystemMetrics(SM_CXSCREEN);
	INT outputHeight = GetSystemMetrics(SM_CYSCREEN);

	INT clientPosX = static_cast<INT>((outputWidth - wndWidth) * 0.5f);
	INT clientPosY = static_cast<INT>((outputHeight - wndHeight) * 0.5f);

	mhMainWnd = CreateWindowW(
		L"MyNewGameEngine", L"MyNewGameEngine",
		WS_OVERLAPPEDWINDOW,
		clientPosX, clientPosY,
		width, height,
		0, 0, mhInst, 0);
	if (!mhMainWnd) ReturnFalse(mpLogFile, L"Failed to create the window");

	ShowWindow(mhMainWnd, SW_SHOW);
	UpdateWindow(mhMainWnd);

	return TRUE;
}