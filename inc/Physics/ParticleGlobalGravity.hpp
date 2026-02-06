#pragma once

#include "ParticleForceGenerator.hpp"

namespace Physics::Cyclone {
	class ParticleGlobalGravity : public ParticleForceGenerator {
	public:
		ParticleGlobalGravity() = default;
		virtual ~ParticleGlobalGravity() = default;

	public:
		virtual void UpdateForce(Particle* pParticle, float dt) override;

	protected:
		DirectX::SimpleMath::Vector3 mGravity{ 0.f, -9.8f, 0.f };
	};
}