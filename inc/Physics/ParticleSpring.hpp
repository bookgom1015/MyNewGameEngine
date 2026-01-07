#pragma once

#include "ParticleForceGenerator.hpp"

namespace Physics::Cyclone {
	class ParticleSpring : public ParticleForceGenerator {
	public:
		ParticleSpring(Particle* pOther, float springConstant, float restLength);
		virtual ~ParticleSpring() = default;

	public:
		virtual void UpdateForce(Particle* pParticle, float dt) override;

	private:
		Particle* mpOther{};

		float mSpringConstant{};
		float mRestLength{};
	};
}