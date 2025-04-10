#ifndef __GAMEWORLD_INL__
#define __GAMEWORLD_INL__

Common::Foundation::Core::WindowsManager* GameWorld::WindowsManager() const {
	return mWindowsManager.get();
}

Common::Input::InputProcessor* GameWorld::InputProcessor() const {
	return mInputProcessor.get();
}

#endif // __GAMEWORLD_INL__