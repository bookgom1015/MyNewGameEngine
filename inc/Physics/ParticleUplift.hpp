#pragma once

#include "ParticleForceGenerator.hpp"

namespace Physics::Cyclone {
	class ParticleUplift : public ParticleForceGenerator {
	public:
		ParticleUplift() = default;
		virtual ~ParticleUplift() = default;

	public:
		virtual void UpdateForce(Particle* pParticle, float dt) override;

	private:
		DirectX::SimpleMath::Vector3 mOrigin{};
		DirectX::SimpleMath::Vector3 mForce{};

		float mRadius{};
	};
}