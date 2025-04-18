#include "GameWorld/Foundation/Core/Component.hpp"
#include "GameWorld/Foundation/Core/Actor.hpp"

using namespace GameWorld::Foundation::Core;

Component::Component(Common::Debug::LogFile* const pLogFile, Actor* const pOwner) {
	mpLogFile = pLogFile;
	mpOwner = pOwner;

	mpOwner->AddComponent(this);
}

const Transform& Component::ActorTransform() { return mpOwner->GetTransform(); }