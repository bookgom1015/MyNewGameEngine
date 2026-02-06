#include "Physics/pch_cyclone.h"
#include "Physics/Particle.hpp"

using namespace Physics::Cyclone;
using namespace DirectX;

void Particle::Integrate(float  dt) {
	if (mInverseMass <= 0.f || dt <= 0.f) return;

	mPosition += mVelocity * dt;

	mVelocity += mAcceleration * dt;
	mVelocity *= std::powf(mDamping, dt);

	mAcceleration = {};
}

void Particle::AddForce(const DirectX::SimpleMath::Vector3& _force) {
	auto force = XMLoadFloat3(&_force);
	mForceAccum += force;
}