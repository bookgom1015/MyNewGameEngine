#pragma once

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
		void CleanUp();

		BOOL ProcessInput(Common::Input::InputState* const pInputState);
		BOOL Update(FLOAT delta);

		void AddActor(Actor* const pActor);
		void RemoveActor(Actor* const pActor);

	private:
		Common::Debug::LogFile* mpLogFile{};

		BOOL mbUpdating{};

		std::vector<std::unique_ptr<Actor>> mActors{};
		std::vector<std::unique_ptr<Actor>> mPendingActors{};

		std::vector<Actor*> mDeadActors{};

		std::unordered_map<std::string, Actor*> mActorRefs{};
	};
}

#include "ActorManager.inl"