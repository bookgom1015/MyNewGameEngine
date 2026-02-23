#include "GameWorld/Foundation/Core/pch_world.h"
#include "GameWorld/Foundation/Mesh/MeshComponent.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Mesh/Mesh.hpp"
#include "Common/Render/Renderer.hpp"
#include "GameWorld/GameWorld.hpp"

using namespace GameWorld::Foundation::Mesh;

MeshComponent::MeshComponent(Common::Debug::LogFile* const pLogFile, Core::Actor* const pOwner)
	: Component(pLogFile, pOwner) {}

MeshComponent::~MeshComponent() {}

bool MeshComponent::OnInitialzing() {
	return true;
}

void MeshComponent::OnCleaningUp() {
	if (mbAddedMesh) GameWorld::GameWorldClass::spGameWorld->Renderer()->RemoveMesh(mMeshHash);
}

bool MeshComponent::ProcessInput(Common::Input::InputState* const pInput) {
	return true;
}

bool MeshComponent::Update(float delta) {
	return true;
}

bool MeshComponent::OnUpdateWorldTransform() {
	auto transform = ActorTransform();
	if (mbAddedMesh) GameWorld::GameWorldClass::spGameWorld->Renderer()->UpdateMeshTransform(mMeshHash, &transform);

	return true;
}

bool MeshComponent::LoadMesh(LPCSTR fileName, LPCSTR baseDir, LPCSTR extension) {
	Common::Foundation::Mesh::Mesh mesh;

	CheckReturn(mpLogFile, Common::Foundation::Mesh::Mesh::Load(mpLogFile, mesh, fileName, baseDir, extension));

	auto transform = ActorTransform();
	CheckReturn(mpLogFile, GameWorld::GameWorldClass::spGameWorld->Renderer()->AddMesh(&mesh, &transform, mMeshHash));

	mbAddedMesh = true;

	return true;
}
