#include "GameWorld/Foundation/Core/pch_world.h"
#include "GameWorld/Foundation/Core/Actor.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Util/MathUtil.hpp"
#include "GameWorld/Foundation/Core/ActorManager.hpp"
#include "GameWorld/GameWorld.hpp"

using namespace GameWorld::Foundation::Core;
using namespace DirectX;

Actor::Actor(
		Common::Debug::LogFile* const pLogFile,
		const std::string& name) 
	: mpLogFile(pLogFile), mName(name) {
	mTransform.Position = SimpleMath::Vector3(0.f);
	mTransform.Rotation = SimpleMath::Vector3(0.f);
	mTransform.Scale = SimpleMath::Vector3(1.f);

	GameWorld::GameWorldClass::spGameWorld->ActorManager()->AddActor(this);
}

Actor::Actor(
		Common::Debug::LogFile* const pLogFile, 
		const std::string& name, 
		const Common::Foundation::Mesh::Transform& trans)
	: mpLogFile(pLogFile), mName(name), mTransform(trans) {
	GameWorld::GameWorldClass::spGameWorld->ActorManager()->AddActor(this);
}

Actor::~Actor() {}

bool Actor::OnInitialzing() { return true; }

bool Actor::ProcessActorInput(Common::Input::InputState* const pInputState) { return true; }

bool Actor::UpdateActor(float delta) { return true; }

bool Actor::UpdateComponents(float delta) {
	for (auto& comp : mComponents) 
		CheckReturn(mpLogFile, comp->Update(delta));

	return true;
}

bool Actor::OnUpdateWorldTransform() {
	if (!mbNeedToUpdate) return true;

	for (size_t i = 0, end = mComponents.size(); i < end; ++i)
		CheckReturn(mpLogFile, mComponents[i]->OnUpdateWorldTransform());

	mbNeedToUpdate = false;

	return true;
}

bool Actor::Initialize() {
	CheckReturn(mpLogFile, OnInitialzing());

	for (const auto& comp : mComponents)
		CheckReturn(mpLogFile, comp->OnInitialzing());

	mbInitialized = true;

	return true;
}

void Actor::CleanUp() {
	for (auto& comp : mComponents)
		comp->OnCleaningUp();
}

bool Actor::ProcessInput(Common::Input::InputState* const pInputState) {
	for (size_t i = 0, end = mComponents.size(); i < end; ++i)
		CheckReturn(mpLogFile, mComponents[i]->ProcessInput(pInputState));

	CheckReturn(mpLogFile, ProcessActorInput(pInputState));

	return true; 
}

bool Actor::Update(float delta) {
	CheckReturn(mpLogFile, OnUpdateWorldTransform());

	CheckReturn(mpLogFile, UpdateComponents(delta));
	CheckReturn(mpLogFile, UpdateActor(delta));

	CheckReturn(mpLogFile, OnUpdateWorldTransform());

	return true; 
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