#include "GameWorld/Prefab/MetalSphere.hpp"
#include "Common/Debug/Logger.hpp"
#include "GameWorld/Foundation/Core/ActorManager.hpp"
#include "GameWorld/Foundation/Core/Component.hpp"
#include "GameWorld/Foundation/Mesh/MeshComponent.hpp"

using namespace GameWorld::Prefab;
using namespace DirectX;

MetalSphere::MetalSphere(
	Common::Debug::LogFile* const pLogFile,
	const std::string& name,
	XMFLOAT3 pos,
	XMFLOAT4 rot,
	XMFLOAT3 scale)
	: Actor(pLogFile, name, pos, rot, scale) {
	mpMeshComp = new GameWorld::Foundation::Mesh::MeshComponent(pLogFile, this);
}

MetalSphere::MetalSphere(
	Common::Debug::LogFile* const pLogFile,
	const std::string& name,
	const Common::Foundation::Mesh::Transform& trans)
	: Actor(pLogFile, name, trans) {
	mpMeshComp = new GameWorld::Foundation::Mesh::MeshComponent(pLogFile, this);
}

BOOL MetalSphere::OnInitialzing() {
	CheckReturn(mpLogFile, mpMeshComp->LoadMesh("metal_sphere", "./../../../assets/Models/", "obj"));

	return TRUE;
}

BOOL MetalSphere::ProcessActorInput(Common::Input::InputState* const pInputState) {
	return TRUE;
}

BOOL MetalSphere::UpdateActor(FLOAT delta) {
	return TRUE;
}
