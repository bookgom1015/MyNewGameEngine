#include "GameWorld/Foundation/Core/Actor.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Util/MathUtil.hpp"
#include "GameWorld/Foundation/Core/ActorManager.hpp"
#include "GameWorld/Foundation/Core/Component.hpp"
#include "GameWorld/GameWorld.hpp"

#include <algorithm>

using namespace GameWorld::Foundation::Core;
using namespace DirectX;

Actor::Actor(
		Common::Debug::LogFile* const pLogFile,
		const std::string& name,
		DirectX::XMFLOAT3 pos,
		DirectX::XMFLOAT4 rot,
		DirectX::XMFLOAT3 scale) 
	: mpLogFile(pLogFile), mName(name) {
	mTransform.Position = XMLoadFloat3(&pos);
	mTransform.Rotation = XMLoadFloat4(&rot);
	mTransform.Scale = XMLoadFloat3(&scale);

	GameWorld::GameWorldClass::spGameWorld->ActorManager()->AddActor(this);
}

Actor::Actor(
		Common::Debug::LogFile* const pLogFile, 
		const std::string& name, 
		const Transform& trans)
	: mpLogFile(pLogFile), mName(name), mTransform(trans) {
	GameWorld::GameWorldClass::spGameWorld->ActorManager()->AddActor(this);
}

BOOL Actor::OnInitialzing() { return TRUE; }

BOOL Actor::ProcessActorInput(Common::Input::InputState* const pInputState) { return TRUE; }

BOOL Actor::UpdateActor(FLOAT delta) { return TRUE; }

BOOL Actor::UpdateComponents(FLOAT delta) {
	for (auto& comp : mComponents) 
		CheckReturn(mpLogFile, comp->Update(delta));

	return TRUE;
}

BOOL Actor::OnUpdateWorldTransform() {
	if (!mbNeedToUpdate) return TRUE;

	for (size_t i = 0, end = mComponents.size(); i < end; ++i)
		CheckReturn(mpLogFile, mComponents[i]->OnUpdateWorldTransform());

	mbNeedToUpdate = FALSE;

	return TRUE;
}

BOOL Actor::Initialize() { 
	CheckReturn(mpLogFile, OnInitialzing());

	for (const auto& comp : mComponents)
		CheckReturn(mpLogFile, comp->OnInitialzing());

	mbInitialized = TRUE;

	return TRUE;
}

void Actor::CleanUp() {
	for (auto& comp : mComponents)
		comp->OnCleaningUp();
}

BOOL Actor::ProcessInput(Common::Input::InputState* const pInputState) { 
	for (size_t i = 0, end = mComponents.size(); i < end; ++i)
		CheckReturn(mpLogFile, mComponents[i]->ProcessInput(pInputState));

	CheckReturn(mpLogFile, ProcessActorInput(pInputState));

	return TRUE; 
}

BOOL Actor::Update(FLOAT delta) { 
	CheckReturn(mpLogFile, OnUpdateWorldTransform());

	CheckReturn(mpLogFile, UpdateComponents(delta));
	CheckReturn(mpLogFile, UpdateActor(delta));

	CheckReturn(mpLogFile, OnUpdateWorldTransform());

	return TRUE; 
}

void Actor::AddComponent(Component* const pComponent) {
	const auto begin = mComponents.begin();
	const auto end = mComponents.end();

	const auto& iter = std::find_if(begin, end, [&](std::unique_ptr<Component>& p) {
		return p.get() == pComponent;
	});

	if (iter != end) return;

	mComponents.push_back(std::unique_ptr<Component>(pComponent));
}

void Actor::RemoveComponent(Component* const pComponent) {
	const auto begin = mComponents.begin();
	const auto end = mComponents.end();

	const auto& iter = std::find_if(begin, end, [&](std::unique_ptr<Component>& p) {
		return p.get() == pComponent;
	});

	if (iter != end) {
		std::iter_swap(iter, end - 1);
		mComponents.pop_back();
	}
}

void Actor::SetPosition(const XMFLOAT3& pos) {
	mbNeedToUpdate = TRUE;
	mTransform.Position = XMLoadFloat3(&pos);
}

void Actor::SetPosition(const XMVECTOR& pos) {
	mbNeedToUpdate = TRUE;
	mTransform.Position = pos;
}

void Actor::AddPosition(const XMFLOAT3& pos) {
	mbNeedToUpdate = TRUE;
	mTransform.Position += XMLoadFloat3(&pos);
}

void Actor::AddPosition(const XMVECTOR& pos) {
	mbNeedToUpdate = TRUE;
	mTransform.Position += pos;
}

void Actor::SetRotation(const XMFLOAT4& rot) {
	mbNeedToUpdate = TRUE;
	mTransform.Rotation = XMLoadFloat4(&rot);
}

void Actor::SetRotation(const XMVECTOR& rot) {
	mbNeedToUpdate = TRUE;
	mTransform.Rotation = rot;
}

void Actor::AddRotation(const XMFLOAT4& rot) {
	mbNeedToUpdate = TRUE;
	mTransform.Rotation = XMQuaternionMultiply(mTransform.Rotation, XMLoadFloat4(&rot));
}

void Actor::AddRotation(const DirectX::XMVECTOR& rot) {
	mbNeedToUpdate = TRUE;
	mTransform.Rotation = XMQuaternionMultiply(mTransform.Rotation, rot);
}

void Actor::AddRotationPitch(FLOAT rad) {
	mbNeedToUpdate = TRUE;
	mTransform.Rotation = XMQuaternionMultiply(mTransform.Rotation, XMQuaternionRotationAxis(UnitVector::RightVector, rad));
}

void Actor::AddRotationYaw(FLOAT rad) {
	mbNeedToUpdate = TRUE;
	mTransform.Rotation = XMQuaternionMultiply(mTransform.Rotation, XMQuaternionRotationAxis(UnitVector::UpVector, rad));
}

void Actor::AddRotationRoll(FLOAT rad) {
	mbNeedToUpdate = TRUE;
	mTransform.Rotation = XMQuaternionMultiply(mTransform.Rotation, XMQuaternionRotationAxis(UnitVector::ForwardVector, rad));
}

void Actor::SetScale(const XMFLOAT3& scale) {
	mbNeedToUpdate = TRUE;
	mTransform.Scale = XMLoadFloat3(&scale);
}

void Actor::SetScale(const XMVECTOR& scale) {
	mbNeedToUpdate = TRUE;
	mTransform.Scale = scale;
}