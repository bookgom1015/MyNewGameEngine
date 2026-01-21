#include "Common/Foundation/Core/WindowsManager.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Input/InputProcessor.hpp"
#include "Common/Foundation/Core/PowerManager.hpp"

#include <string>

#define ID_BTN_HALT		201
#define ID_BTN_SELECT	202

#define ID_RADIO_BTN_0	1000
#define ID_RADIO_BTN_1	1001
#define ID_RADIO_BTN_2	1002
#define ID_RADIO_BTN_3	1003
#define ID_RADIO_BTN_4	1004
#define ID_RADIO_BTN_5	1005
#define ID_RADIO_BTN_6	1006
#define ID_RADIO_BTN_7	1007

using namespace Common::Foundation::Core;

INT WindowsManager::DialogBaseUnits = GetDialogBaseUnits();

namespace {
	const INT DialogBoxWidth = 300;
	const INT ButtonWidth = 80;
	const INT ButtonHeight = 30;
	const INT RadioButtonHeight = 40;

	LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		// Forward hwnd on because we can get messages (e.g., WM_CREATE)
		// before CreateWindow returns, and thus before mhMainWnd is valid
		return WindowsManager::sWindowsManager->MsgProc(hWnd, msg, wParam, lParam);
	}

	INT_PTR CALLBACK DialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
		static WindowsManager::SelectDialogInitData* initData = nullptr;
		static std::vector<HWND> radioButtons;

		switch (msg) {
		case WM_INITDIALOG: {
			initData = reinterpret_cast<WindowsManager::SelectDialogInitData*>(lParam);

			const auto dialogBoxHeight = static_cast<INT>((initData->Items.size() + 2) * RadioButtonHeight);
			const auto itemCount = initData->Items.size();

			// Set caption title
			if (!SetWindowTextW(hDlg, L"Select GPU")) break;

			// Halt button
			{
				CreateWindow(
					TEXT("BUTTON"),
					TEXT("Halt"),
					WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
					15,										// X
					dialogBoxHeight - ButtonHeight - 15,	// Y
					ButtonWidth,							// Width
					ButtonHeight,							// Height
					hDlg, (HMENU)ID_BTN_HALT, initData->InstanceHandle, nullptr);
			}
			// Select button
			{
				CreateWindow(
					TEXT("BUTTON"), 
					TEXT("Select"),
					WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
					DialogBoxWidth - ButtonWidth - 15,		// X
					dialogBoxHeight - ButtonHeight - 15,	// Y
					ButtonWidth,							// Width
					ButtonHeight,							// Height
					hDlg, (HMENU)ID_BTN_SELECT, initData->InstanceHandle, nullptr);
			}
			// Radio buttons
			for (size_t i = 0; i < itemCount; ++i) {
				const INT yPos = static_cast<INT>(15 + i * RadioButtonHeight);
				const HWND hRadioButton = CreateWindowW(
					TEXT("BUTTON"), 
					initData->Items[i].c_str(),
					WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | (i == 0 ? WS_GROUP : 0),
					15,						// X
					yPos,					// Y
					DialogBoxWidth - 30,	// Width
					30,						// Height
					hDlg, (HMENU)(ID_RADIO_BTN_0 + i), initData->InstanceHandle, nullptr);
				radioButtons.push_back(hRadioButton);
			}

			initData->SelectedItemIndex = 0;
			return CheckRadioButton(hDlg, ID_RADIO_BTN_0, ID_RADIO_BTN_0 + static_cast<INT>(itemCount), ID_RADIO_BTN_0);
		}
		case WM_ERASEBKGND: {
			HDC hdc = (HDC)wParam;
			RECT rc;
			if (!GetClientRect(hDlg, &rc)) break;

			HBRUSH hBrush = static_cast<HBRUSH>(GetStockObject(DKGRAY_BRUSH));
			FillRect(hdc, &rc, hBrush);
			return TRUE;
		}
		case WM_CLOSE: {
			EndDialog(hDlg, 0);
			break;
		}
		case WM_COMMAND: {
			if (LOWORD(wParam) == ID_BTN_HALT) {
				initData->Halted = TRUE;

				EndDialog(hDlg, 0);
			}
			else if (LOWORD(wParam) == ID_BTN_SELECT) {
				for (size_t i = 0, end = initData->Items.size(); i < end; ++i) {
					if (SendMessageW(radioButtons[i], BM_GETCHECK, 0, 0) == BST_CHECKED) {
						initData->SelectedItemIndex = static_cast<UINT>(i);
						break;
					}
				}

				EndDialog(hDlg, 0);
			}
			break;
		}
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

	mPowerManager = std::make_unique<PowerManager>();
}

WindowsManager::~WindowsManager() {}

INT WindowsManager::ToDLUsWidth(INT width) { return (width * 4) / LOWORD(DialogBaseUnits); }

INT WindowsManager::ToDLUsHeight(INT height) { return (height * 8) / HIWORD(DialogBaseUnits); }

LRESULT CALLBACK WindowsManager::MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (mbMsgCallbackHooked) {
		if (mMsgCallBackFunc(hWnd, msg, wParam, lParam)) return TRUE;
	}

	switch (msg) {
		// WM_ACTIVATE is sent when the window is activated or deactivated.  
		// We pause the game when the window is deactivated and unpause it 
		// when it becomes active.
	case WM_ACTIVATE: {
		BOOL sleepable = mPowerManager->IsSleepable();

		if (LOWORD(wParam) == WA_INACTIVE) {
			if (!sleepable) mPowerManager->SetMode(FALSE);

			mbAppPaused = TRUE;
		}
		else {
			if (sleepable) mPowerManager->SetMode(TRUE);

			mbAppPaused = FALSE;
		}
		return 0;
	}
					// WM_SIZE is sent when the user resizes the window.  
	case WM_SIZE: {
		// Save the new client area dimensions.
		UINT width = LOWORD(lParam);
		UINT height = HIWORD(lParam);

		BOOL sleepable = mPowerManager->IsSleepable();
		if (wParam == SIZE_MINIMIZED) {
			if (!sleepable) mPowerManager->SetMode(FALSE);

			mbAppPaused = TRUE;
			mbMinimized = TRUE;
			mbMaximized = FALSE;
		}
		else if (wParam == SIZE_MAXIMIZED) {
			if (sleepable) mPowerManager->SetMode(FALSE);

			mbAppPaused = FALSE;
			mbMinimized = FALSE;
			mbMaximized = TRUE;
			OnResize(width, height);
		}
		else if (wParam == SIZE_RESTORED) {
			// Restoring from minimized state?
			if (mbMinimized) {
				if (sleepable) mPowerManager->SetMode(FALSE);

				mbAppPaused = FALSE;
				mbMinimized = FALSE;
				OnResize(width, height);
			}

			// Restoring from maximized state?
			else if (mbMaximized) {
				if (sleepable) mPowerManager->SetMode(FALSE);

				mbAppPaused = FALSE;
				mbMaximized = FALSE;
				OnResize(width, height);
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
				OnResize(width, height);
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

		OnResize(width, height);
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

void WindowsManager::UnregisterOnResizeFunc() {
	mOnResizeFunc = {};
	mbRegisteredOnResizeFunc = FALSE;
}

void WindowsManager::DestroyWindow() {
	mbDestroyed = TRUE;
	PostQuitMessage(0);
}

void WindowsManager::RegisterInputProcessor(Common::Input::InputProcessor* const pInputProc) {
	mpInputProcessor = pInputProc;
	mbInputProcessorRegistered = TRUE;
}

void WindowsManager::HookMsgCallback(const MsgCallBackFunc& func) {
	mMsgCallBackFunc = func;
	mbMsgCallbackHooked = TRUE;
}

void WindowsManager::UnhookMsgCallback() {
	mMsgCallBackFunc = {};
	mbMsgCallbackHooked = FALSE;
}

BOOL WindowsManager::SelectDialog(SelectDialogInitData* const pInitData) {
	DLGTEMPLATE* dialogTemplate;

	BYTE dlgBuffer[1024] = { 0 };
	dialogTemplate = (DLGTEMPLATE*)dlgBuffer;

	RECT rc{};
	if (!GetClientRect(mhMainWnd, &rc)) rc = { 0, 0, 0, 0 };

	const INT parentW = rc.right - rc.left;
	const INT parentH = rc.bottom - rc.top;

	const INT dialogBoxHeight = static_cast<INT>((pInitData->Items.size() + 2) * RadioButtonHeight);

	const INT clientPosX = ToDLUsWidth(static_cast<INT>((parentW - DialogBoxWidth) * 0.5f));
	const INT clientPosY = ToDLUsWidth(static_cast<INT>((parentH - dialogBoxHeight) * 0.5f));

	dialogTemplate->style = WS_POPUP | WS_BORDER | DS_MODALFRAME | WS_CAPTION;
	dialogTemplate->dwExtendedStyle = 0;
	dialogTemplate->cdit = 0;
	dialogTemplate->x = clientPosX;
	dialogTemplate->y = clientPosY;
	dialogTemplate->cx = ToDLUsWidth(DialogBoxWidth);
	dialogTemplate->cy = ToDLUsHeight(dialogBoxHeight);

	DialogBoxIndirectParamW(mhInst, dialogTemplate, mhMainWnd, DialogProc, reinterpret_cast<LPARAM>(pInitData));

	if (pInitData->Halted) ReturnFalse(mpLogFile, "The game has been halted");

	return TRUE;
}

BOOL WindowsManager::InitMainWnd(UINT wndWidth, UINT wndHeight) {
	mMainWndWidth = wndWidth;
	mMainWndHeight = wndHeight;

	WNDCLASS wc{};
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
	if (!AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, FALSE)) ReturnFalse(mpLogFile, L"Failed to get window rect");

	const INT width = R.right - R.left;
	const INT height = R.bottom - R.top;

	const INT outputWidth = GetSystemMetrics(SM_CXSCREEN);
	const INT outputHeight = GetSystemMetrics(SM_CYSCREEN);

	const INT clientPosX = static_cast<INT>((outputWidth - wndWidth) * 0.5f);
	const INT clientPosY = static_cast<INT>((outputHeight - wndHeight) * 0.5f);

	mhMainWnd = CreateWindowW(
		L"MyNewGameEngine", L"MyNewGameEngine",
		WS_OVERLAPPEDWINDOW,
		clientPosX, clientPosY,
		width, height,
		0, 0, mhInst, 0);
	if (!mhMainWnd) ReturnFalse(mpLogFile, L"Failed to create the window");

	ShowWindow(mhMainWnd, SW_SHOW);
	if (!UpdateWindow(mhMainWnd)) ReturnFalse(mpLogFile, L"Failed to update window");

	return TRUE;
}

void WindowsManager::OnResize(UINT width, UINT height) {
	if (mMainWndWidth != width || mMainWndHeight != height) {
		mMainWndWidth = width;
		mMainWndHeight = height;

		if (mbRegisteredOnResizeFunc) mOnResizeFunc(width, height);
	}
}