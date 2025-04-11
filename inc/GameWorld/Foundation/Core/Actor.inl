#ifndef __ACTOR_INL__
#define __ACTOR_INL__

constexpr const std::string& GameWorld::Foundation::Core::Actor::Name() const {
	return mName;
}

constexpr const Transform& GameWorld::Foundation::Core::Actor::GetTransform() const {
	return mTransform;
}

constexpr BOOL GameWorld::Foundation::Core::Actor::Initialized() const {
	return mbInitialized;
}

constexpr BOOL GameWorld::Foundation::Core::Actor::IsDead() const {
	return mbIsDead;
}

#endif // __ACTOR_INL__