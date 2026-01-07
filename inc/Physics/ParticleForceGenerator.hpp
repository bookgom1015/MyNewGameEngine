#pragma once

#include <DirectXMath.h>

namespace Physics::Cyclone {
	class Particle;

	class ParticleForceGenerator {
	public:
		ParticleForceGenerator() = default;
		virtual ~ParticleForceGenerator() = default;

	public:
		virtual void UpdateForce(Particle* pParticle, float dt) = 0;
	};
}