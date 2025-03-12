#include "GameWorld/GameWorld.hpp"
#include "Common/Debug/Logger.hpp"

INT WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine , _In_ INT nCmdShow) {
	if (!Debug::Logger::Initialize()) return -1;

	try {
		GameWorld gw;
		
		CheckReturn(gw.Initialize());
		CheckReturn(gw.RunLoop());
		gw.CleanUp();

		WLogln(L"GameWorld successfully shotdowned");

		return 0;
	}
	catch (std::exception& e) {
		Logln("GameWorld catched exception: ", e.what());
		return -1;
	}
}