#include "GameWorld/Foundation/Core/pch_world.h"
#include "GameWorld/Prefab/MetalSphere.hpp"
#include "Common/Debug/Logger.hpp"
#include "GameWorld/Foundation/Core/ActorManager.hpp"
#include "GameWorld/Foundation/Core/Component.hpp"
#include "GameWorld/Foundation/Mesh/MeshComponent.hpp"

using namespace GameWorld::Prefab;
using namespace DirectX;

MetalSphere::MetalSphere(
	Common::Debug::LogFile* const pLogFile,
	const std::string& name)
	: Actor(pLogFile, name) {
	mpMeshComp = new GameWorld::Foundation::Mesh::MeshComponent(pLogFile, this);
}

MetalSphere::~MetalSphere() {}

MetalSphere::MetalSphere(
	Common::Debug::LogFile* const pLogFile,
	const std::string& name,
	const Common::Foundation::Mesh::Transform& trans)
	: Actor(pLogFile, name, trans) {
	mpMeshComp = new GameWorld::Foundation::Mesh::MeshComponent(pLogFile, this);
}

bool MetalSphere::OnInitialzing() {
	CheckReturn(mpLogFile, mpMeshComp->LoadMesh("metal_sphere", "./../../../assets/Models/", "obj"));

	return true;
}

bool MetalSphere::ProcessActorInput(Common::Input::InputState* const pInputState) {
	return true;
}

bool MetalSphere::UpdateActor(float delta) {
	return true;
}
