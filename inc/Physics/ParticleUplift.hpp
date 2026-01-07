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
		DirectX::XMVECTOR mOrigin{};
		DirectX::XMVECTOR mForce{};

		float mRadius{};
	};
}