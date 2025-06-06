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
			const std::string& name,
			DirectX::XMFLOAT3 pos = { 0.f, 0.f, 0.f },
			DirectX::XMFLOAT4 rot = { 0.f, 0.f, 0.f, 1.f },
			DirectX::XMFLOAT3 scale = { 1.f, 1.f, 1.f });
		MetalSphere(
			Common::Debug::LogFile* const pLogFile,
			const std::string& name,
			const Common::Foundation::Mesh::Transform& trans);
		virtual ~MetalSphere() = default;

	protected:
		virtual BOOL OnInitialzing() override;
		virtual BOOL ProcessActorInput(Common::Input::InputState* const pInputState) override;
		virtual BOOL UpdateActor(FLOAT delta) override;

	private:
		GameWorld::Foundation::Mesh::MeshComponent* mpMeshComp;
	};
}