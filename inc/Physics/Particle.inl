#ifndef __PARTICLE_INL__
#define __PARTICLE_INL__

namespace Physics::Cyclone {
	constexpr float Particle::GetMass() const noexcept {
		return 1.f / mInverseMass;
	}

	constexpr void Particle::SetMass(float  mass) noexcept {
		mInverseMass = 1.f / std::max(mass, 1e-6f);
	}

	constexpr float Particle::GetInverseMass() const noexcept {
		return mInverseMass;
	}

	constexpr void Particle::SetInverseMass(float  invMass) noexcept { 
		mInverseMass = invMass;
	}

	constexpr bool Particle::HasFiniteMass() const { return mInverseMass > 0.f; }

	void Particle::SetDamping(float damping) noexcept {
		mDamping = damping;
	}

	void Particle::SetAcceleration(const DirectX::SimpleMath::Vector3& accel) {
		mAcceleration = accel;
	}

	DirectX::SimpleMath::Vector3& Particle::GetAcceleration() {
		return mAcceleration;
	}

	const DirectX::SimpleMath::Vector3& Particle::GetAcceleration() const {
		return mAcceleration;
	}

	void Particle::SetVelocity(const DirectX::SimpleMath::Vector3& vel) {
		mVelocity = vel;
	}

	DirectX::SimpleMath::Vector3& Particle::GetVelocity() { return mVelocity; }

	const DirectX::SimpleMath::Vector3& Particle::GetVelocity() const { return mVelocity; }

	void Particle::SetPosition(const DirectX::SimpleMath::Vector3& pos) {
		mPosition = pos;
	}

	DirectX::SimpleMath::Vector3& Particle::GetPosition() { return mPosition; }

	const DirectX::SimpleMath::Vector3& Particle::GetPosition() const { return mPosition; }

	void Particle::ClearAccumulator() noexcept {
		mForceAccum = {};
	}
}

#endif // __PARTICLE_INL__