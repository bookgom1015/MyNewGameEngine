#pragma once

#include "ParticleDrag.hpp"

namespace Physics::Cyclone {
	class ParticleAirBrake : public ParticleDrag {
	public:
		ParticleAirBrake() = default;
		virtual ~ParticleAirBrake() = default;

	public:
		virtual void UpdateForce(Particle* pParticle, float dt) override;

	private:
		bool mbBrake{};
	};
}