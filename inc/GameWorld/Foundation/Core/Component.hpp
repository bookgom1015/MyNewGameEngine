#pragma once

#include <Windows.h>

#include "Common/Foundation/Mesh/Transform.hpp"

namespace Common::Input {
	struct InputState;
}

namespace GameWorld::Foundation::Core {
	class Actor;

	class Component {
	public:
		Component(Actor* const pOwner);
		virtual ~Component() = default;

	public:
		virtual BOOL OnInitialzing();
		virtual BOOL ProcessInput(Common::Input::InputState* const pInput) = 0;
		virtual BOOL Update(FLOAT delta) = 0;
		virtual BOOL OnUpdateWorldTransform() = 0;

	protected:
		const Transform& ActorTransform();

	private:
		Actor* mOwner;
	};
}