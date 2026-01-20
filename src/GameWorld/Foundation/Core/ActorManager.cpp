#include "GameWorld/Foundation/Core/pch_world.h"
#include "GameWorld/Foundation/Core/ActorManager.hpp"
#include "Common/Debug/Logger.hpp"
#include "GameWorld/Foundation/Core/Actor.hpp"

using namespace GameWorld::Foundation::Core;

ActorManager::ActorManager() {}

ActorManager::~ActorManager() {}

BOOL ActorManager::Initialize(Common::Debug::LogFile* const pLogFile) {
	mpLogFile = pLogFile;

	return TRUE;
}

void ActorManager::CleanUp() {
	for (auto& actor : mPendingActors)
		actor->CleanUp();

	for (auto& actor : mActors)
		actor->CleanUp();
}

BOOL ActorManager::ProcessInput(Common::Input::InputState* const pInputState) {	
	for (size_t i = 0, end = mActors.size(); i < end; ++i)
		CheckReturn(mpLogFile, mActors[i]->ProcessInput(pInputState));

	return TRUE;
}

BOOL ActorManager::Update(FLOAT delta) {
	for (auto& actor : mPendingActors)
		mActors.push_back(std::move(actor));
	mPendingActors.clear();

	mbUpdating = TRUE;
	for (size_t i = 0, end = mActors.size(); i < end; ++i) {
		if (!mActors[i]->Initialized())
			CheckReturn(mpLogFile, mActors[i]->Initialize());
	}
	for (size_t i = 0, end = mActors.size(); i < end; ++i) {
		CheckReturn(mpLogFile, mActors[i]->Update(delta));

		if (mActors[i]->IsDead()) 
			mDeadActors.push_back(mActors[i].get());
	}
	mbUpdating = FALSE;

	for (const auto actor : mDeadActors) {
		const auto begin = mActors.begin();
		const auto end = mActors.end();

		const auto& iter = std::find_if(begin, end, [&](std::unique_ptr<Actor>& p) {
			return p.get() == actor;
		});
		if (iter != end) {
			mActorRefs.erase(actor->Name());

			std::iter_swap(iter, end - 1);
			mActors.pop_back();
		}
	}
	mDeadActors.clear();

	return TRUE;
}


void ActorManager::AddActor(Actor* const pActor) {
	if (mbUpdating) {
		const auto begin = mPendingActors.begin();
		const auto end = mPendingActors.end();

		const auto& iter = std::find_if(begin, end, [&](std::unique_ptr<Actor>& p) {
			return p.get() == pActor;
		});
		if (iter != end) return;

		mPendingActors.push_back(std::unique_ptr<Actor>(pActor));
	}
	else {
		const auto begin = mActors.begin();
		const auto end = mActors.end();

		const auto& iter = std::find_if(begin, end, [&](std::unique_ptr<Actor>& p) {
			return p.get() == pActor;
		});
		if (iter != end) return;

		mActors.push_back(std::unique_ptr<Actor>(pActor));
	}

	mActorRefs[pActor->Name()] = pActor;
}

void ActorManager::RemoveActor(Actor* const pActor) {
	const auto begin = mDeadActors.begin();
	const auto end = mDeadActors.end();

	const auto iter = std::find(begin, end, pActor);
	if (iter != end) return;

	mDeadActors.push_back(pActor);
}