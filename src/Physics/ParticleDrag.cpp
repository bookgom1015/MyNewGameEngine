#include "Physics/pch_cyclone.h"
#include "Physics/ParticleDrag.hpp"
#include "Physics/Particle.hpp"

using namespace Physics::Cyclone;
using namespace DirectX;

ParticleDrag::ParticleDrag(float k1, float k2) { mK1 = k1; mK2 = k2; }

void ParticleDrag::UpdateForce(Particle* pParticle, float dt) {
	auto velocity = pParticle->GetVelocity();

	auto dragCoeff = XMVectorGetX(XMVector3Length(velocity));
	dragCoeff = mK1 * dragCoeff + mK2 * dragCoeff * dragCoeff;

	auto force = XMVector3Normalize(velocity);
	force *= -dragCoeff;

	pParticle->AddForce(force);
}