#include "Physics/pch_cyclone.h"
#include "Physics/ParticleSpring.hpp"
#include "Physics/Particle.hpp"

using namespace Physics::Cyclone;
using namespace DirectX;

ParticleSpring::ParticleSpring(
	Particle* pOther, float springConstant, float restLength) 
	: mpOther{ pOther }, mSpringConstant{ springConstant }, mRestLength{ restLength } {}

void ParticleSpring::UpdateForce(Particle* pParticle, float dt) {
	auto diff = pParticle->GetPosition() - mpOther->GetPosition();

	auto magnitude = diff.Length();
	magnitude = std::abs(magnitude - mRestLength);
	magnitude *= mSpringConstant;

	auto force = diff;
	force.Normalize();
	force *= magnitude;

	pParticle->AddForce(force);
}