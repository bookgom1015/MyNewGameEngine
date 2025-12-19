#include "Physics/Particle.hpp"

#include <cmath>

using namespace DirectX;
using namespace Physics::Cyclone;

void Particle::Integrate(FLOAT dt) {
	if (mInverseMass <= 0.f && dt <= 0.f) return;

	auto position = XMLoadFloat3(&mPosition);
	auto velocity = XMLoadFloat3(&mVelocity);
	position += velocity * dt;
	XMStoreFloat3(&mPosition, position);

	auto accel = XMLoadFloat3(&mAcceleration);
	velocity += accel * dt;
	velocity *= std::powf(mDamping, dt);
	XMStoreFloat3(&mVelocity, velocity);

	mAcceleration = { 0.f, 0.f, 0.f };
}

void Particle::AddForce(const DirectX::XMFLOAT3& _force) {
	auto forceAccum = XMLoadFloat3(&mForceAccum);
	auto force = XMLoadFloat3(&_force);
	forceAccum += force;

	XMStoreFloat3(&mForceAccum, forceAccum);
}