#pragma once

#include "GameWorld/Foundation/Core/Actor.hpp"

namespace GameWorld::Foundation::Mesh {
	class MeshComponent;
}

namespace GameWorld::Prefab {
	class MetalSphere : public Foundation::Core::Actor {
	public:
		MetalSphere(
			Common::Debug::LogFile* const pLogFile,
			const std::string& name);
		MetalSphere(
			Common::Debug::LogFile* const pLogFile,
			const std::string& name,
			const Common::Foundation::Mesh::Transform& trans);
		virtual ~MetalSphere();

	protected:
		virtual bool OnInitialzing() override;
		virtual bool ProcessActorInput(Common::Input::InputState* const pInputState) override;
		virtual bool UpdateActor(float delta) override;

	private:
		GameWorld::Foundation::Mesh::MeshComponent* mpMeshComp{};
	};
}