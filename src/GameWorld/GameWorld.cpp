#include "GameWorld/GameWorld.hpp"
#include "Common/Debug/Logger.hpp"

namespace {
	LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		// Forward hwnd on because we can get messages (e.g., WM_CREATE)
		// before CreateWindow returns, and thus before mhMainWnd is valid
		return GameWorld::sGameWorld->MsgProc(hwnd, msg, wParam, lParam);
	}

	UINT InitClientWidth = 1280;
	UINT InitClientHeight = 720;


	UINT InputFrameCount  = 0;
	UINT UpdateFrameCount = 0;
	UINT DrawFrameCount	  = 0;
}

GameWorld* GameWorld::sGameWorld = nullptr;

GameWorld::GameWorld() {
	sGameWorld = this;
}

GameWorld::~GameWorld() {
	CleanUp();
}

BOOL GameWorld::Initialize() {
	CheckReturn(CreateMainWindow());

	return TRUE;
}

BOOL GameWorld::RunLoop() {
	MSG msg = { 0 };

	std::vector<std::thread> threads;

	threads.emplace_back(&GameWorld::ProcessInput, this);
	threads.emplace_back(&GameWorld::Update, this);
	threads.emplace_back(&GameWorld::Draw, this);

	while (msg.message != WM_QUIT) {
		// If there are Window messages then process them
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// Otherwise, do animation/game stuff
		else {
			if (mStage == ProcessingStage::E_DrawFinished) {
				{
					std::lock_guard<std::mutex> lock(mStageMutex);
					mStage = ProcessingStage::E_InputReady;
				}
				mInputCV.notify_one();
			}
			else if (mStage == ProcessingStage::E_InputFinished) {
				{
					std::lock_guard<std::mutex> lock(mStageMutex);
					mStage = ProcessingStage::E_UpdateReady;
				}
				mUpdateCV.notify_one();
			}
			else if (mStage == ProcessingStage::E_UpdateFinished) {
				{
					std::lock_guard<std::mutex> lock(mStageMutex);
					mStage = ProcessingStage::E_DrawReady;
				}
				mDrawCV.notify_one();
			}
		}
	}

	mInputCV.notify_one();
	mUpdateCV.notify_one();
	mDrawCV.notify_one();
	for (auto& thread : threads)
		thread.join();

	WLogln(L"Input Frame Count: \t\t", std::to_wstring(InputFrameCount));
	WLogln(L"Updpate Frame Count: \t", std::to_wstring(UpdateFrameCount));
	WLogln(L"Draw Frame Count: \t\t", std::to_wstring(DrawFrameCount));

	return TRUE;
}

void GameWorld::CleanUp() {

}

LRESULT GameWorld::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	static UINT width = InitClientWidth;
	static UINT height = InitClientHeight;

	//if ((mGameState == EGameStates::EGS_UI) && ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam)) return 0;

	switch (msg) {
		// WM_ACTIVATE is sent when the window is activated or deactivated.  
		// We pause the game when the window is deactivated and unpause it 
		// when it becomes active.
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE) {
			bAppPaused = TRUE;
		}
		else {
			bAppPaused = FALSE;
		}
		return 0;

		// WM_SIZE is sent when the user resizes the window.  
	case WM_SIZE:
	{
		// Save the new client area dimensions.
		width = LOWORD(lParam);
		height = HIWORD(lParam);
		if (wParam == SIZE_MINIMIZED) {
			bAppPaused = TRUE;
			bMinimized = TRUE;
			bMaximized = FALSE;
		}
		else if (wParam == SIZE_MAXIMIZED) {
			bAppPaused = FALSE;
			bMinimized = FALSE;
			bMaximized = TRUE;
			OnResize(width, height);
		}
		else if (wParam == SIZE_RESTORED) {
			// Restoring from minimized state?
			if (bMinimized) {
				bAppPaused = FALSE;
				bMinimized = FALSE;
				OnResize(width, height);
			}
		
			// Restoring from maximized state?
			else if (bMaximized) {
				bAppPaused = FALSE;
				bMaximized = FALSE;
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
			else if (bResizing) {
		
			}
			// API call such as SetWindowPos or mSwapChain->SetFullscreenState.
			else {
				OnResize(width, height);
			}
		}
		return 0;
	}

	// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
		bAppPaused = TRUE;
		bResizing = TRUE;
		return 0;

		// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
		// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:
		bAppPaused = FALSE;
		bResizing = FALSE;
		OnResize(width, height);
		return 0;

		// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY:
		bDestroying = TRUE;
		PostQuitMessage(0);
		return 0;

		// The WM_MENUCHAR message is sent when a menu is active and the user presses 
		// a key that does not correspond to any mnemonic or accelerator key. 
	case WM_MENUCHAR:
		// Don't beep when we alt-enter.
		return MAKELRESULT(0, MNC_CLOSE);

		// Catch this message so to prevent the window from becoming too small.
	case WM_GETMINMAXINFO:
		return 0;

	case WM_KEYUP:
		return 0;

	case WM_KEYDOWN:
		return 0;

	case WM_LBUTTONDOWN: {
		return 0;
	}
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

BOOL GameWorld::CreateMainWindow() {
	WNDCLASS wc;
	wc.style		 = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc	 = MainWndProc;
	wc.cbClsExtra	 = 0;
	wc.cbWndExtra	 = 0;
	wc.hInstance	 = mhInst;
	wc.hIcon		 = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor		 = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(DKGRAY_BRUSH));
	wc.lpszMenuName	 = 0;
	wc.lpszClassName = L"MyNewGameEngine";
	if (!RegisterClass(&wc)) ReturnFalse(L"Failed to register the window class");

	// Compute window rectangle dimensions based on requested client area dimensions.
	RECT R = { 0, 0, static_cast<LONG>(InitClientWidth), static_cast<LONG>(InitClientHeight) };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, FALSE);
	INT width  = R.right - R.left;
	INT height = R.bottom - R.top;

	INT outputWidth  = GetSystemMetrics(SM_CXSCREEN);
	INT outputHeight = GetSystemMetrics(SM_CYSCREEN);

	INT clientPosX = static_cast<INT>((outputWidth  - InitClientWidth)  * 0.5f);
	INT clientPosY = static_cast<INT>((outputHeight - InitClientHeight) * 0.5f);

	mhMainWnd = CreateWindow(
		L"MyNewGameEngine", L"MyNewGameEngine",
		WS_OVERLAPPEDWINDOW,
		clientPosX, clientPosY,
		width, height,
		0, 0, mhInst, 0);
	if (!mhMainWnd) ReturnFalse(L"Failed to create the window");

	ShowWindow(mhMainWnd, SW_SHOW);
	UpdateWindow(mhMainWnd);

	return TRUE;
}

void GameWorld::OnResize(UINT width, UINT height) {
	
}

BOOL GameWorld::ProcessInput() {
	while (!bDestroying) {
		std::unique_lock<std::mutex> lock(mStageMutex);
		mInputCV.wait(lock, [&] { return (mStage == ProcessingStage::E_InputReady) || bDestroying; });
		if (bDestroying) break;

		++InputFrameCount;

		mStage = ProcessingStage::E_InputFinished;
	}

	return TRUE;
}

BOOL GameWorld::Update() {
	while (!bDestroying) {
		std::unique_lock<std::mutex> lock(mStageMutex);
		mUpdateCV.wait(lock, [&] { return (mStage == ProcessingStage::E_UpdateReady) || bDestroying; });
		if (bDestroying) break;

		++UpdateFrameCount;

		mStage = ProcessingStage::E_UpdateFinished;
	}

	return TRUE;
}

BOOL GameWorld::Draw() {
	while (!bDestroying) {
		std::unique_lock<std::mutex> lock(mStageMutex);
		mDrawCV.wait(lock, [&] { return (mStage == ProcessingStage::E_DrawReady) || bDestroying; });
		if (bDestroying) break;

		++DrawFrameCount;

		mStage = ProcessingStage::E_DrawFinished;
	}

	return TRUE;
}
