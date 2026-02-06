#include "Physics/pch_cyclone.h"
#include "Physics/ParticlePointGravity.hpp"
#include "Physics/Particle.hpp"

using namespace Physics::Cyclone;
using namespace DirectX;

void ParticlePointGravity::UpdateForce(Particle* pParticle, float dt) {
	auto diff = mGravityPoint - pParticle->GetPosition();
	auto dist = diff.Length();
	auto dir = diff;
	dir.Normalize();

	auto dist2 = dist * dist;
	auto magnitude = mMagnitude / dist2;

	mGravity = dir * magnitude;

	ParticleGlobalGravity::UpdateForce(pParticle, dt);
}