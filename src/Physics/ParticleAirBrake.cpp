#include "Physics/pch_cyclone.h"
#include "Physics/ParticleAirBrake.hpp"

using namespace Physics::Cyclone;

void ParticleAirBrake::UpdateForce(Particle* pParticle, float dt) {
	if (mbBrake) ParticleDrag::UpdateForce(pParticle, dt);
}