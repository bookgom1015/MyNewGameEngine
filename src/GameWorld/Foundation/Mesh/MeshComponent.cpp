#include "GameWorld/Foundation/Mesh/MeshComponent.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Mesh/Mesh.hpp"
#include "Common/Render/Renderer.hpp"
#include "GameWorld/GameWorld.hpp"

using namespace GameWorld::Foundation::Mesh;

MeshComponent::MeshComponent(Common::Debug::LogFile* const pLogFile, Core::Actor* const pOwner)
	: Component(pLogFile, pOwner) {}

BOOL MeshComponent::OnInitialzing() {
	return TRUE;
}

void MeshComponent::OnCleaningUp() {
	GameWorld::GameWorldClass::spGameWorld->Renderer()->RemoveMesh();
}

BOOL MeshComponent::ProcessInput(Common::Input::InputState* const pInput) {
	return TRUE;
}

BOOL MeshComponent::Update(FLOAT delta) {
	return TRUE;
}

BOOL MeshComponent::OnUpdateWorldTransform() {
	return TRUE;
}

BOOL MeshComponent::LoadMesh(LPCSTR fileName, LPCSTR baseDir) {
	Common::Foundation::Mesh::Mesh mesh;
	CheckReturn(mpLogFile, Common::Foundation::Mesh::Mesh::LoadObj(mpLogFile, mesh, fileName, baseDir));

	CheckReturn(mpLogFile, GameWorld::GameWorldClass::spGameWorld->Renderer()->AddMesh(&mesh));

	return TRUE;
}
