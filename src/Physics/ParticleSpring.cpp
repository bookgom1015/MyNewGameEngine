#include "Physics/ParticleSpring.hpp"
#include "Physics/Particle.hpp"

using namespace Physics::Cyclone;
using namespace DirectX;

ParticleSpring::ParticleSpring(
	Particle* pOther, float springConstant, float restLength) 
	: mpOther{ pOther }, mSpringConstant{ springConstant }, mRestLength{ restLength } {}

void ParticleSpring::UpdateForce(Particle* pParticle, float dt) {
	auto diff = XMVectorSubtract(pParticle->GetPosition(), mpOther->GetPosition());

	auto magnitude = XMVectorGetX(XMVector3Length(diff));
	magnitude = std::abs(magnitude - mRestLength);
	magnitude *= mSpringConstant;

	auto force = XMVector3Normalize(diff);
	force *= magnitude;

	pParticle->AddForce(force);
}