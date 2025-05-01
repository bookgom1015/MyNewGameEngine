#include "GameWorld/GameWorld.hpp"
#include "Common/Debug/Logger.hpp"

#include <memory>

void CreateDebuggingConsole() {
	AllocConsole();

	FILE* fp;
	freopen_s(&fp, "CONOUT$", "w", stdout);
	freopen_s(&fp, "CONOUT$", "w", stderr);

	SetConsoleOutputCP(CP_UTF8);

	ConsoleLog("Debugging Console Initialized");
}

void DestroyDebuggingConsole(BOOL bNeedToPause = FALSE) {
	ConsoleLog("Debugging Console will be terminated");
	if (bNeedToPause) system("pause");
	FreeConsole();
}

INT WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine , _In_ INT nCmdShow) {
#ifdef _DEBUG
	CreateDebuggingConsole();
#endif

	std::unique_ptr<Common::Debug::LogFile> LogFile = std::make_unique<Common::Debug::LogFile>();
	const auto logFile = LogFile.get();

	if (!Common::Debug::Logger::Initialize(logFile, L"./log.txt")) return -1;

	try {
		GameWorld::GameWorldClass gw;

		if (!gw.Initialize(logFile, hInstance)) {
			WLogln(logFile, L"Failed to initialize game");
#ifdef _DEBUG
			DestroyDebuggingConsole(TRUE);
#endif
			return -1;
		}
		if (!gw.RunLoop()) {
			WLogln(logFile, L"Game-loop broke");
#ifdef _DEBUG
			DestroyDebuggingConsole(TRUE);
#endif
			return -1;
		}
		gw.CleanUp();

		WLogln(logFile, L"Game successfully cleaned up");
#ifdef _DEBUG
		DestroyDebuggingConsole();
#endif

		return 0;
	}
	catch (std::exception& e) {
		Logln(logFile, "Catched exception: ", e.what());
#ifdef _DEBUG
		DestroyDebuggingConsole(TRUE);
#endif
		return -1;
	}
}