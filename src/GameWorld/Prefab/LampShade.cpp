#include "GameWorld/Foundation/Core/pch_world.h"
#include "GameWorld/Prefab/LampShade.hpp"
#include "Common/Debug/Logger.hpp"
#include "GameWorld/Foundation/Core/ActorManager.hpp"
#include "GameWorld/Foundation/Core/Component.hpp"
#include "GameWorld/Foundation/Mesh/MeshComponent.hpp"

using namespace GameWorld::Prefab;
using namespace DirectX;

LampShade::LampShade(
		Common::Debug::LogFile* const pLogFile,
		const std::string& name) 
	: Actor(pLogFile, name) {
	mpMeshComp = new GameWorld::Foundation::Mesh::MeshComponent(pLogFile, this);
}

LampShade::LampShade(
		Common::Debug::LogFile* const pLogFile, 
		const std::string& name, 
		const Common::Foundation::Mesh::Transform& trans)
	: Actor(pLogFile, name, trans) {
	mpMeshComp = new GameWorld::Foundation::Mesh::MeshComponent(pLogFile, this);
}

LampShade::~LampShade() {}

bool LampShade::OnInitialzing() {
	CheckReturn(mpLogFile, mpMeshComp->LoadMesh("field", "./../../../assets/Models/", "obj"));

	return true;
}

bool LampShade::ProcessActorInput(Common::Input::InputState* const pInputState) {
	return true;
}

bool LampShade::UpdateActor(float delta) {
	return true;
}
