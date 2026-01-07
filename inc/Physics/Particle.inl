#ifndef __PARTICLE_INL__
#define __PARTICLE_INL__

namespace Physics::Cyclone {
	constexpr float Particle::GetMass() const noexcept {
		return 1.f / mInverseMass;
	}

	constexpr void Particle::SetMass(float  mass) noexcept {
		mInverseMass = 1.f / std::max(mass, 1e-6f);
	}

	constexpr void Particle::SetInverseMass(float  invMass) noexcept { 
		mInverseMass = invMass;
	}

	constexpr bool Particle::HasFiniteMass() const { return mInverseMass > 0.f; }

	DirectX::XMVECTOR& Particle::GetVelocity() { return mVelocity; }

	const DirectX::XMVECTOR& Particle::GetVelocity() const { return mVelocity; }

	DirectX::XMVECTOR& Particle::GetPosition() { return mPosition; }

	const DirectX::XMVECTOR& Particle::GetPosition() const { return mPosition; }
}

#endif // __PARTICLE_INL__