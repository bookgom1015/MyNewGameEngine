#pragma once

#include "GameWorld/Foundation/Core/Actor.hpp"

namespace GameWorld::Foundation::Mesh {
	class MeshComponent;
}

namespace GameWorld::Prefab {
	class FineDonut : public Foundation::Core::Actor {
	public:
		FineDonut(
			Common::Debug::LogFile* const pLogFile,
			const std::string& name);
		FineDonut(
			Common::Debug::LogFile* const pLogFile,
			const std::string& name,
			const Common::Foundation::Mesh::Transform& trans);
		virtual ~FineDonut();

	protected:
		virtual bool OnInitialzing() override;
		virtual bool ProcessActorInput(Common::Input::InputState* const pInputState) override;
		virtual bool UpdateActor(float delta) override;

	private:
		GameWorld::Foundation::Mesh::MeshComponent* mpMeshComp{};
	};
}