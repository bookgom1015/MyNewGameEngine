#include "GameWorld/Foundation/Core/pch_world.h"
#include "GameWorld/Prefab/FineDonut.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Util/MathUtil.hpp"
#include "GameWorld/Foundation/Core/ActorManager.hpp"
#include "GameWorld/Foundation/Core/Component.hpp"
#include "GameWorld/Foundation/Mesh/MeshComponent.hpp"

using namespace GameWorld::Prefab;
using namespace DirectX;
using namespace DirectX::SimpleMath;

FineDonut::FineDonut(
		Common::Debug::LogFile* const pLogFile,
		const std::string& name)
	: Actor(pLogFile, name) {
	mpMeshComp = new GameWorld::Foundation::Mesh::MeshComponent(pLogFile, this);
}

FineDonut::FineDonut(
	Common::Debug::LogFile* const pLogFile,
	const std::string& name,
	const Common::Foundation::Mesh::Transform& trans)
	: Actor(pLogFile, name, trans) {
	mpMeshComp = new GameWorld::Foundation::Mesh::MeshComponent(pLogFile, this);
}

FineDonut::~FineDonut() {}

bool FineDonut::OnInitialzing() {
	CheckReturn(mpLogFile, mpMeshComp->LoadMesh("fine_donut", "./../../../assets/Models/", "obj"));

	return true;
}

bool FineDonut::ProcessActorInput(Common::Input::InputState* const pInputState) {
	return true;
}

bool FineDonut::UpdateActor(float delta) {
	static float elapsed = 0.f;
	elapsed += delta;

	float degree = elapsed * 45.f;

	SetRotation(Vector3(degree, degree, 0.f));

	return true;
}
