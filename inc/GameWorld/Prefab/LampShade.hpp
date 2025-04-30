#pragma once

#include "GameWorld/Foundation/Core/Actor.hpp"

namespace GameWorld::Foundation::Mesh {
	class MeshComponent;
}

namespace GameWorld::Prefab {
	class LampShade : public Foundation::Core::Actor {
	public:
		LampShade(
			Common::Debug::LogFile* const pLogFile,
			const std::string& name,
			DirectX::XMFLOAT3 pos = { 0.f, 0.f, 0.f },
			DirectX::XMFLOAT4 rot = { 0.f, 0.f, 0.f, 1.f },
			DirectX::XMFLOAT3 scale = { 1.f, 1.f, 1.f });
		LampShade(
			Common::Debug::LogFile* const pLogFile, 
			const std::string& name, 
			const Common::Foundation::Mesh::Transform& trans);
		virtual ~LampShade() = default;

	protected:
		virtual BOOL OnInitialzing() override;
		virtual BOOL ProcessActorInput(Common::Input::InputState* const pInputState) override;
		virtual BOOL UpdateActor(FLOAT delta) override;

	private:
		GameWorld::Foundation::Mesh::MeshComponent* mpMeshComp;
	};
}