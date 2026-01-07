#include "Physics/ParticleUplift.hpp"
#include "Physics/Particle.hpp"

using namespace Physics::Cyclone;
using namespace DirectX;

void ParticleUplift::UpdateForce(Particle* pParticle, float dt) {
	auto pos = pParticle->GetPosition();
	auto diffV = pos - mOrigin;
	diffV = XMVectorSetY(diffV, 0.f);

	auto diff = XMVectorGetX(XMVector3Length(diffV));

	auto diff2 = diff * diff;
	auto radius2 = mRadius * mRadius;
	if (diff2 < radius2) pParticle->AddForce(mForce);
}