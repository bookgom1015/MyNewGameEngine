#include "GameWorld/GameWorld.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Core/HWInfo.hpp"
#include "Common/Foundation/Core/GameTimer.hpp"
#include "Common/Render/Renderer.hpp"
#include "Common/Input/InputProcessor.hpp"

namespace {
	LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		// Forward hwnd on because we can get messages (e.g., WM_CREATE)
		// before CreateWindow returns, and thus before mhMainWnd is valid
		return GameWorld::spGameWorld->mInputProcessor->MsgProc(hwnd, msg, wParam, lParam);
	}

	UINT InitClientWidth = 1280;
	UINT InitClientHeight = 720;

	UINT InputFrameCount  = 0;
	UINT UpdateFrameCount = 0;
	UINT DrawFrameCount	  = 0;

	typedef Common::Render::Renderer* (*CreateRendererFunc)();
	typedef void (*DestroyRendererFunc)(Common::Render::Renderer*);

	typedef Common::Input::InputProcessor* (*CreateInputProcessorFunc)();
	typedef void (*DestroyInputProcessorFunc)(Common::Input::InputProcessor*);
}

GameWorld* GameWorld::spGameWorld = nullptr;

GameWorld::GameWorld() {
	spGameWorld = this;

	mGameTimer = std::make_unique<Common::Foundation::Core::GameTimer>();
}

GameWorld::~GameWorld() {
	CleanUp();
}

BOOL GameWorld::Initialize(Common::Debug::LogFile* const pLogFile) {
	mpLogFile = pLogFile;

	CheckReturn(mpLogFile, GetHWInfo());
	CheckReturn(mpLogFile, CreateInputProcessor());
	CheckReturn(mpLogFile, CreateMainWindow());
	CheckReturn(mpLogFile, CreateRenderer());

	mGameTimer->SetFrameTimeLimit(Common::Foundation::Core::GameTimer::FrameTimeLimits::E_60f);

	bInitialized = TRUE;

	return TRUE;
}

BOOL GameWorld::RunLoop() {
	MSG msg = { 0 };

	std::vector<std::thread> threads;

	threads.emplace_back(&GameWorld::ProcessInput, this);
	threads.emplace_back(&GameWorld::Update, this);
	threads.emplace_back(&GameWorld::Draw, this);

	mGameTimer->Reset();

	FLOAT currTime = 0.f;
	FLOAT prevTime = 0.f;

	while (msg.message != WM_QUIT) {
		// If there are Window messages then process them
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// Otherwise, do animation/game stuff
		else {
			mGameTimer->Tick();

			if (mStage == ProcessingStage::E_DrawFinished) {
				currTime = mGameTimer->TotalTime();
				if (currTime - prevTime < mGameTimer->FrameTimeLimit()) continue;

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

				prevTime = mGameTimer->TotalTime();
			}
		}
	}

	mInputCV.notify_one();
	mUpdateCV.notify_one();
	mDrawCV.notify_one();

	for (auto& thread : threads)
		thread.join();

#ifdef _DEBUG
	WLogln(mpLogFile, L"Input Frame Count: \t\t", std::to_wstring(InputFrameCount));
	WLogln(mpLogFile, L"Updpate Frame Count: \t", std::to_wstring(UpdateFrameCount));
	WLogln(mpLogFile, L"Draw Frame Count: \t\t", std::to_wstring(DrawFrameCount));
#endif

	return TRUE;
}

void GameWorld::CleanUp() {}

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
	if (!RegisterClass(&wc)) ReturnFalse(mpLogFile, L"Failed to register the window class");
	
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
	if (!mhMainWnd) ReturnFalse(mpLogFile, L"Failed to create the window");
	
	ShowWindow(mhMainWnd, SW_SHOW);
	UpdateWindow(mhMainWnd);
	
	return TRUE;
}

BOOL GameWorld::GetHWInfo() {
	Common::Foundation::Core::Processor processor;
	Common::Foundation::Core::HWInfo::ProcessorInfo(mpLogFile, processor);

#ifdef _DEBUG
	const auto& YesOrNo = [](BOOL state) {
		return state ? L"YES" : L"NO";
		};

	WLogln(mpLogFile, L"--------------------------------------------------------------------");
	WLogln(mpLogFile, L"Processor: ", processor.Name.c_str());
	WLogln(mpLogFile, L"Instruction support:");
	WLogln(mpLogFile, L"    MMX: ", YesOrNo(processor.SupportMMX));
	WLogln(mpLogFile, L"    SSE: ", YesOrNo(processor.SupportSSE));
	WLogln(mpLogFile, L"    SSE2: ", YesOrNo(processor.SupportSSE2));
	WLogln(mpLogFile, L"    SSE3: ", YesOrNo(processor.SupportSSE3));
	WLogln(mpLogFile, L"    SSSE3: ", YesOrNo(processor.SupportSSSE3));
	WLogln(mpLogFile, L"    SSE4.1: ", YesOrNo(processor.SupportSSE4_1));
	WLogln(mpLogFile, L"    SSE4.2: ", YesOrNo(processor.SupportSSE4_2));
	WLogln(mpLogFile, L"    AVX: ", YesOrNo(processor.SupportAVX));
	WLogln(mpLogFile, L"    AVX2: ", YesOrNo(processor.SupportAVX2));
	WLogln(mpLogFile, L"    AVX512: ", YesOrNo(processor.SupportAVX512F && processor.SupportAVX512DQ && processor.SupportAVX512BW));
	WLogln(mpLogFile, L"Physical core count: ", std::to_wstring(processor.Physical));
	WLogln(mpLogFile, L"Logical core count: ", std::to_wstring(processor.Logical));
	WLogln(mpLogFile, L"Total physical memory: ", std::to_wstring(processor.TotalPhysicalMemory), L"MB");
	WLogln(mpLogFile, L"Total virtual memory: ", std::to_wstring(processor.TotalVirtualMemory), L"MB");
	WLogln(mpLogFile, L"--------------------------------------------------------------------");
#endif

	return TRUE;
}

BOOL GameWorld::CreateRenderer() {
	const auto hRendererDLL = LoadLibraryW(L"Renderer.dll");
	if (!hRendererDLL) ReturnFalse(mpLogFile, L"Failed to load Renderer.dll");

	CreateRendererFunc createFunc = (CreateRendererFunc)GetProcAddress(hRendererDLL, "CreateRenderer");
	DestroyRendererFunc destroyFunc = (DestroyRendererFunc)GetProcAddress(hRendererDLL, "DestroyRenderer");
	if (!createFunc || !destroyFunc) ReturnFalse(mpLogFile, L"Failed to find Renderer.dll functions");

	mRenderer = std::unique_ptr<Common::Render::Renderer>(createFunc());
	CheckReturn(mpLogFile, mRenderer->Initialize(mpLogFile, mhMainWnd, InitClientWidth, InitClientHeight));

	return TRUE;
}

BOOL GameWorld::CreateInputProcessor() {
	const auto hInputDLL = LoadLibraryW(L"Input.dll");
	if (!hInputDLL) ReturnFalse(mpLogFile, L"Failed to load Input.dll");

	CreateInputProcessorFunc createFunc = (CreateInputProcessorFunc)GetProcAddress(hInputDLL, "CreateInputProcessor");
	DestroyInputProcessorFunc destroyFunc = (DestroyInputProcessorFunc)GetProcAddress(hInputDLL, "DestroyInputProcessor");
	if (!createFunc || !destroyFunc) ReturnFalse(mpLogFile, L"Failed to find Input.dll functions");

	mInputProcessor = std::unique_ptr<Common::Input::InputProcessor>(createFunc());
	CheckReturn(mpLogFile, mInputProcessor->Initialize(mpLogFile));

	mInputProcessor->RegisterOnResizeFunc(std::bind(&GameWorld::OnResize, this, std::placeholders::_1, std::placeholders::_2));

	return TRUE;
}

void GameWorld::OnResize(UINT width, UINT height) {
	if (!bInitialized) return;
	if (!mRenderer->OnResize(width, height)) {
		WLogln(mpLogFile, L"Resizing failed");
		mInputProcessor->Halt();
	}
#ifdef _DEBUG
	std::cout << "Resized (Width: " << width << " Height: " << height << ")" << std::endl;
#endif
}

BOOL GameWorld::ProcessInput() {
	while (!mInputProcessor->Destroying()) {
		std::unique_lock<std::mutex> lock(mStageMutex);
		mInputCV.wait(lock, [&] { return (mStage == ProcessingStage::E_InputReady) || mInputProcessor->Destroying(); });
		if (mInputProcessor->Destroying()) break;

		//

		++InputFrameCount;
		mStage = ProcessingStage::E_InputFinished;
	}

	return TRUE;
}

BOOL GameWorld::Update() {
	while (!mInputProcessor->Destroying()) {
		std::unique_lock<std::mutex> lock(mStageMutex);
		mUpdateCV.wait(lock, [&] { return (mStage == ProcessingStage::E_UpdateReady) || mInputProcessor->Destroying(); });
		if (mInputProcessor->Destroying()) break;

		//

		++UpdateFrameCount;
		mStage = ProcessingStage::E_UpdateFinished;
	}

	return TRUE;
}

BOOL GameWorld::Draw() {
	while (!mInputProcessor->Destroying()) {
		std::unique_lock<std::mutex> lock(mStageMutex);
		mDrawCV.wait(lock, [&] { return (mStage == ProcessingStage::E_DrawReady) || mInputProcessor->Destroying(); });
		if (mInputProcessor->Destroying()) break;

		// 

		++DrawFrameCount;
		mStage = ProcessingStage::E_DrawFinished;
	}

	return TRUE;
}
