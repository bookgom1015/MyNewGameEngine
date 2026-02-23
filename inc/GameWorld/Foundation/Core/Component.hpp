#pragma once

#include "Common/Foundation/Mesh/Transform.hpp"
#include "Common/Util/HashUtil.hpp"

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

	class Component {
	public:
		Component(Common::Debug::LogFile* const pLogFile, Actor* const pOwner);
		virtual ~Component();

	public:
		virtual bool OnInitialzing() = 0;
		virtual void OnCleaningUp() = 0;

		virtual bool ProcessInput(Common::Input::InputState* const pInput) = 0;
		virtual bool Update(float delta) = 0;
		virtual bool OnUpdateWorldTransform() = 0;

	protected:
		const Common::Foundation::Mesh::Transform& ActorTransform();

	protected:
		Common::Debug::LogFile* mpLogFile{};
		Actor* mpOwner{};
	};
}