#pragma once

#include "Common/Foundation/Mesh/Transform.hpp"
#include "Common/Util/HashUtil.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <wrl.h>
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

	class Component {
	public:
		Component(Common::Debug::LogFile* const pLogFile, Actor* const pOwner);
		virtual ~Component() = default;

	public:
		virtual BOOL OnInitialzing() = 0;
		virtual void OnCleaningUp() = 0;

		virtual BOOL ProcessInput(Common::Input::InputState* const pInput) = 0;
		virtual BOOL Update(FLOAT delta) = 0;
		virtual BOOL OnUpdateWorldTransform() = 0;

	protected:
		const Transform& ActorTransform();

	protected:
		Common::Debug::LogFile* mpLogFile = nullptr;
		Actor* mpOwner = nullptr;
	};
}