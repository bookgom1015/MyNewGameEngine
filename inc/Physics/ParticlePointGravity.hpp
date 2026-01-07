#pragma once

#include "ParticleGlobalGravity.hpp"

namespace Physics::Cyclone {
	class ParticlePointGravity : public ParticleGlobalGravity {
	public:
		ParticlePointGravity() = default;
		virtual ~ParticlePointGravity() = default;

	public:
		virtual void UpdateForce(Particle* pParticle, float dt) override;

	private:
		DirectX::XMVECTOR mGravityPoint{};

		float mMagnitude{ 9.8f };
	};
}