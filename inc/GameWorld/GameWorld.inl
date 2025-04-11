#ifndef __GAMEWORLD_INL__
#define __GAMEWORLD_INL__

Common::Foundation::Core::WindowsManager* GameWorld::GameWorldClass::WindowsManager() const { return mWindowsManager.get(); }

GameWorld::Foundation::Core::ActorManager* GameWorld::GameWorldClass::ActorManager() const { return mActorManager.get(); }

Common::Input::InputProcessor* GameWorld::GameWorldClass::InputProcessor() const { return mInputProcessor.get(); }

Common::Render::Renderer* GameWorld::GameWorldClass::Renderer() const { return mRenderer.get(); }

#endif // __GAMEWORLD_INL__