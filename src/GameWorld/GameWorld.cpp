#include "GameWorld/GameWorld.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Core/WindowsManager.hpp"
#include "Common/Foundation/Core/HWInfo.hpp"
#include "Common/Foundation/Core/GameTimer.hpp"
#include "Common/Render/Renderer.hpp"
#include "Common/Render/ShadingArgument.hpp"
#include "Common/Input/InputProcessor.hpp"
#include "GameWorld/Foundation/Core/ActorManager.hpp"
#include "GameWorld/Player/FreeLookActor.hpp"

using namespace GameWorld;

namespace {
	const UINT InitClientWidth = 1280;
	const UINT InitClientHeight = 720;

	UINT InputFrameCount  = 0;
	UINT UpdateFrameCount = 0;
	UINT DrawFrameCount	  = 0;

	typedef Common::Render::Renderer* (*CreateRendererFunc)();
	typedef void (*DestroyRendererFunc)(Common::Render::Renderer*);

	typedef Common::Input::InputProcessor* (*CreateInputProcessorFunc)();
	typedef void (*DestroyInputProcessorFunc)(Common::Input::InputProcessor*);
}

GameWorldClass* GameWorldClass::spGameWorld = nullptr;

GameWorldClass::GameWorldClass() {
	spGameWorld = this;

	mWindowsManager = std::make_unique<Common::Foundation::Core::WindowsManager>();
	mActorManager = std::make_unique<GameWorld::Foundation::Core::ActorManager>();
	mArgumentSet = std::make_unique<Common::Render::ShadingArgument::ShadingArgumentSet>();
	mGameTimer = std::make_unique<Common::Foundation::Core::GameTimer>();
}

GameWorldClass::~GameWorldClass() {
	CleanUp();
}

BOOL GameWorldClass::Initialize(Common::Debug::LogFile* const pLogFile, HINSTANCE hInstance) {
	mpLogFile = pLogFile;

	CheckReturn(mpLogFile, BuildHWInfo());
	CheckReturn(mpLogFile, InitWindowsManager(hInstance));
	CheckReturn(mpLogFile, InitActorManager());
	CheckReturn(mpLogFile, CreateInputProcessor());
	CheckReturn(mpLogFile, CreateRenderer());

	mWindowsManager->RegisterOnResizeFunc(std::bind(&GameWorldClass::OnResize, this, std::placeholders::_1, std::placeholders::_2));
	mWindowsManager->RegisterInputProcessor(mInputProcessor.get());

	mGameTimer->SetFrameTimeLimit(Common::Foundation::Core::GameTimer::FrameTimeLimits::E_90f);

	bInitialized = TRUE;

	return TRUE;
}

BOOL GameWorldClass::RunLoop() {
	MSG msg = { 0 };

	std::vector<std::thread> threads;
	threads.emplace_back(&GameWorldClass::ProcessInput, this);
	threads.emplace_back(&GameWorldClass::Update, this);
	threads.emplace_back(&GameWorldClass::Draw, this);

	mGameTimer->Reset();

	FLOAT currTime = 0.f;
	FLOAT prevTime = 0.f;

	CheckReturn(mpLogFile, BuildScene());

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
	WLogln(mpLogFile, L"Input Frame Count: ", std::to_wstring(InputFrameCount));
	WLogln(mpLogFile, L"Updpate Frame Count: ", std::to_wstring(UpdateFrameCount));
	WLogln(mpLogFile, L"Draw Frame Count: ", std::to_wstring(DrawFrameCount));
#endif

	return TRUE;
}

void GameWorldClass::CleanUp() {
	mActorManager->CleanUp();
}

BOOL GameWorldClass::BuildHWInfo() {
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

BOOL GameWorldClass::InitWindowsManager(HINSTANCE hInstance) {
	CheckReturn(mpLogFile, mWindowsManager->Initialize(mpLogFile, hInstance, InitClientWidth, InitClientHeight));

	return TRUE;
}

BOOL GameWorldClass::InitActorManager() {
	CheckReturn(mpLogFile, mActorManager->Initialize(mpLogFile));

	return TRUE;
}

BOOL GameWorldClass::CreateRenderer() {
	const auto hRendererDLL = LoadLibraryW(L"Renderer.dll");
	if (!hRendererDLL) ReturnFalse(mpLogFile, L"Failed to load Renderer.dll");

	CreateRendererFunc createFunc = (CreateRendererFunc)GetProcAddress(hRendererDLL, "CreateRenderer");
	DestroyRendererFunc destroyFunc = (DestroyRendererFunc)GetProcAddress(hRendererDLL, "DestroyRenderer");
	if (!createFunc || !destroyFunc) ReturnFalse(mpLogFile, L"Failed to find Renderer.dll functions");

	mRenderer = std::unique_ptr<Common::Render::Renderer>(createFunc());
	CheckReturn(mpLogFile, mRenderer->Initialize(mpLogFile, mWindowsManager.get(), mArgumentSet.get(), InitClientWidth, InitClientHeight));

	return TRUE;
}

BOOL GameWorldClass::CreateInputProcessor() {
	const auto hInputDLL = LoadLibraryW(L"Input.dll");
	if (!hInputDLL) ReturnFalse(mpLogFile, L"Failed to load Input.dll");

	CreateInputProcessorFunc createFunc = (CreateInputProcessorFunc)GetProcAddress(hInputDLL, "CreateInputProcessor");
	DestroyInputProcessorFunc destroyFunc = (DestroyInputProcessorFunc)GetProcAddress(hInputDLL, "DestroyInputProcessor");
	if (!createFunc || !destroyFunc) ReturnFalse(mpLogFile, L"Failed to find Input.dll functions");

	mInputProcessor = std::unique_ptr<Common::Input::InputProcessor>(createFunc());
	CheckReturn(mpLogFile, mInputProcessor->Initialize(mpLogFile));

	return TRUE;
}

BOOL GameWorldClass::BuildScene() {
	new Player::FreeLookActor(mpLogFile, "free_look_actor");

	return TRUE;
}

void GameWorldClass::OnResize(UINT width, UINT height) {
	if (!bInitialized) return;
	if (!mRenderer->OnResize(width, height)) {
		WLogln(mpLogFile, L"Resizing failed");
		mWindowsManager->DestroyWindow();
	}
}

BOOL GameWorldClass::ProcessInput() {
	while (!mWindowsManager->Destroyed()) {
		std::unique_lock<std::mutex> lock(mStageMutex);
		mInputCV.wait(lock, [&] { return (mStage == ProcessingStage::E_InputReady) || mWindowsManager->Destroyed(); });
		if (mWindowsManager->Destroyed()) break;

		auto inputState = mInputProcessor->GetInputState();
		CheckReturn(mpLogFile, mActorManager->ProcessInput(&inputState));

		++InputFrameCount;
		mStage = ProcessingStage::E_InputFinished;
	}

	return TRUE;
}

BOOL GameWorldClass::Update() {
	while (!mWindowsManager->Destroyed()) {
		std::unique_lock<std::mutex> lock(mStageMutex);
		mUpdateCV.wait(lock, [&] { return (mStage == ProcessingStage::E_UpdateReady) || mWindowsManager->Destroyed(); });
		if (mWindowsManager->Destroyed()) break;

		const auto dt = mGameTimer->DeltaTime();

		CheckReturn(mpLogFile, mActorManager->Update(dt));
		CheckReturn(mpLogFile, mRenderer->Update(dt));

		++UpdateFrameCount;
		mStage = ProcessingStage::E_UpdateFinished;
	}

	return TRUE;
}

BOOL GameWorldClass::Draw() {
	while (!mWindowsManager->Destroyed()) {
		std::unique_lock<std::mutex> lock(mStageMutex);
		mDrawCV.wait(lock, [&] { return (mStage == ProcessingStage::E_DrawReady) || mWindowsManager->Destroyed(); });
		if (mWindowsManager->Destroyed()) break;

		CheckReturn(mpLogFile, mRenderer->Draw());

		++DrawFrameCount;
		mStage = ProcessingStage::E_DrawFinished;
	}

	return TRUE;
}
