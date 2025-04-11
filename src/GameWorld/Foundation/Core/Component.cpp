#include "GameWorld/Foundation/Core/Component.hpp"
#include "GameWorld/Foundation/Core/Actor.hpp"

using namespace GameWorld::Foundation::Core;

Component::Component(Actor* const pOwner) {
	mOwner = pOwner;
	pOwner->AddComponent(this);
}

BOOL Component::OnInitialzing() { return TRUE; }

const Transform& Component::ActorTransform() { return mOwner->GetTransform(); }