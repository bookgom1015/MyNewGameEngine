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

void DestroyDebuggingConsole() {
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
		GameWorld gw;
		
		CheckReturn(logFile, gw.Initialize(logFile));
		CheckReturn(logFile, gw.RunLoop());
		gw.CleanUp();

		WLogln(logFile, L"Game successfully shotdowned");

		return 0;
	}
	catch (std::exception& e) {
		Logln(logFile, "Game catched exception: ", e.what());
		return -1;
	}
	
#ifdef _DEBUG
	DestroyDebuggingConsole();
#endif
}