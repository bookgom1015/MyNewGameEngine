#include "Physics/ParticlePointGravity.hpp"
#include "Physics/Particle.hpp"

using namespace Physics::Cyclone;
using namespace DirectX;

void ParticlePointGravity::UpdateForce(Particle* pParticle, float dt) {
	auto diff = XMVectorSubtract(mGravityPoint, pParticle->GetPosition());
	auto dist = XMVectorGetX(XMVector3Length(diff));
	auto dir = XMVector3Normalize(diff);

	auto dist2 = dist * dist;
	auto magnitude = mMagnitude / dist2;

	mGravity = dir * magnitude;

	ParticleGlobalGravity::UpdateForce(pParticle, dt);
}