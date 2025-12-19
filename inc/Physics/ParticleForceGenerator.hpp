#pragma once

#include <windef.h>

namespace Physics::Cyclone {
	class Particle;

	class ParticleForceGenerator {
	public:
		ParticleForceGenerator() = default;
		virtual ~ParticleForceGenerator() = default;

	public:
		virtual void UpdateForce(Particle* pParticle, FLOAT dt) = 0;
	};
}