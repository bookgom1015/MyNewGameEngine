#ifndef __ACTORMANAGER_INL__
#define __ACTORMANAGER_INL__

GameWorld::Foundation::Core::Actor* GameWorld::Foundation::Core::ActorManager::GetActor(const std::string& name) {
	return mActorRefs[name];
}

#endif // __ACTORMANAGER_INL__