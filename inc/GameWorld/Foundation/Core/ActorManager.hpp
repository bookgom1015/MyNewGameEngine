#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <Windows.h>

namespace Common {
	namespace Debug {
		struct LogFile;
	}

	namespace Input {
		struct InputState;
	}	
}

namespace GameWorld::Foundation::Core {
	class Actor;

	class ActorManager {
	public:
		ActorManager();
		virtual ~ActorManager();

	public:
		__forceinline Actor* GetActor(const std::string& name);

	public:
		BOOL Initialize(Common::Debug::LogFile* const pLogFile);

	public:
		BOOL ProcessInput(Common::Input::InputState* const pInputState);
		BOOL Update(FLOAT delta);

		void AddActor(Actor* const pActor);
		void RemoveActor(Actor* const pActor);

	private:
		BOOL mbUpdating = FALSE;

		Common::Debug::LogFile* mpLogFile = nullptr;

		std::vector<std::unique_ptr<Actor>> mActors;
		std::vector<std::unique_ptr<Actor>> mPendingActors;
		std::vector<Actor*> mDeadActors;

		std::unordered_map<std::string, Actor*> mActorRefs;
	};
}

#include "ActorManager.inl"