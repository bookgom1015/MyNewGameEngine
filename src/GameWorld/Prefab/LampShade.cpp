#include "GameWorld/Prefab/LampShade.hpp"
#include "Common/Debug/Logger.hpp"
#include "GameWorld/Foundation/Core/ActorManager.hpp"
#include "GameWorld/Foundation/Core/Component.hpp"
#include "GameWorld/Foundation/Mesh/MeshComponent.hpp"

using namespace GameWorld::Prefab;
using namespace DirectX;

LampShade::LampShade(
		Common::Debug::LogFile* const pLogFile,
		const std::string& name,
		XMFLOAT3 pos,
		XMFLOAT4 rot,
		XMFLOAT3 scale) 
	: Actor(pLogFile, name, pos, rot, scale) {
	mpMeshComp = new GameWorld::Foundation::Mesh::MeshComponent(pLogFile, this);
}

LampShade::LampShade(
		Common::Debug::LogFile* const pLogFile, 
		const std::string& name, 
		const Transform& trans)
	: Actor(pLogFile, name, trans) {
	mpMeshComp = new GameWorld::Foundation::Mesh::MeshComponent(pLogFile, this);
}

BOOL LampShade::OnInitialzing() {
	CheckReturn(mpLogFile, mpMeshComp->LoadMesh("lampshade", "./../../../../assets/Models/", "obj"));

	return TRUE;
}

BOOL LampShade::ProcessActorInput(Common::Input::InputState* const pInputState) {
	return TRUE;
}

BOOL LampShade::UpdateActor(FLOAT delta) {
	return TRUE;
}
