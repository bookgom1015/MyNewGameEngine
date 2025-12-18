#include "GameWorld/Prefab/FineDonut.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Util/MathUtil.hpp"
#include "GameWorld/Foundation/Core/ActorManager.hpp"
#include "GameWorld/Foundation/Core/Component.hpp"
#include "GameWorld/Foundation/Mesh/MeshComponent.hpp"

using namespace GameWorld::Prefab;
using namespace DirectX;

FineDonut::FineDonut(
		Common::Debug::LogFile* const pLogFile,
		const std::string& name,
		XMFLOAT3 pos,
		XMFLOAT4 rot,
		XMFLOAT3 scale)
	: Actor(pLogFile, name, pos, rot, scale) {
	mpMeshComp = new GameWorld::Foundation::Mesh::MeshComponent(pLogFile, this);
}

FineDonut::FineDonut(
	Common::Debug::LogFile* const pLogFile,
	const std::string& name,
	const Common::Foundation::Mesh::Transform& trans)
	: Actor(pLogFile, name, trans) {
	mpMeshComp = new GameWorld::Foundation::Mesh::MeshComponent(pLogFile, this);
}

BOOL FineDonut::OnInitialzing() {
	CheckReturn(mpLogFile, mpMeshComp->LoadMesh("fine_donut", "./../../../assets/Models/", "obj"));

	return TRUE;
}

BOOL FineDonut::ProcessActorInput(Common::Input::InputState* const pInputState) {
	return TRUE;
}

BOOL FineDonut::UpdateActor(FLOAT delta) {
	static FLOAT elapsed = 0.f;
	elapsed += delta;

	auto rad = Common::Util::MathUtil::DegreesToRadians(elapsed * 45.f);

	auto q1 = XMQuaternionRotationAxis(UnitVector::UpVector, rad);
	auto q2 = XMQuaternionMultiply(UnitVector::RightVector, q1);

	XMFLOAT4 rot;
	XMStoreFloat4(&rot, XMQuaternionRotationAxis(q2, rad));

	SetRotation(rot);

	return TRUE;
}
