#include "GameWorld/Foundation/Core/pch_world.h"
#include "GameWorld/Foundation/Core/Component.hpp"
#include "GameWorld/Foundation/Core/Actor.hpp"

using namespace GameWorld::Foundation::Core;

Component::Component(Common::Debug::LogFile* const pLogFile, Actor* const pOwner) {
	mpLogFile = pLogFile;
	mpOwner = pOwner;

	mpOwner->AddComponent(this);
}

Component::~Component() {}

const Common::Foundation::Mesh::Transform& Component::ActorTransform() { return mpOwner->GetTransform(); }