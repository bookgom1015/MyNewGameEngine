#include "Physics/Particle.hpp"

#include <cmath>

using namespace DirectX;
using namespace Physics::Cyclone;

void Particle::Integrate(float  dt) {
	if (mInverseMass <= 0.f || dt <= 0.f) return;

	mPosition += mVelocity * dt;

	mVelocity += mAcceleration * dt;
	mVelocity *= std::powf(mDamping, dt);

	mAcceleration = XMVectorSet(0.f, 0.f, 0.f, 0.f);
}

void Particle::AddForce(const XMFLOAT3& _force) {
	auto force = XMLoadFloat3(&_force);
	mForceAccum += force;
}

void Particle::AddForce(const DirectX::XMVECTOR& force) {
	mForceAccum += force;
}