#pragma once

#include "ParticleForceGenerator.hpp"

namespace Physics::Cyclone {
	class ParticleDrag : public ParticleForceGenerator {
	public:
		ParticleDrag(float k1, float k2);
		virtual ~ParticleDrag() = default;

	public:
		virtual void UpdateForce(Particle* pParticle, float dt) override;

	private:
		float mK1{};
		float mK2{};
	};
}