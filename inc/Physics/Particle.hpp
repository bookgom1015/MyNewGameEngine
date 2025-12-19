#pragma once

#include <DirectXMath.h>

#include <windef.h>

namespace Physics::Cyclone {
	class Particle {
	public:
		Particle() = default;
		virtual ~Particle() = default;

	public:
		__forceinline constexpr void SetMass(FLOAT mass);
		__forceinline constexpr void SetInverseMass(FLOAT invMass);

		void Integrate(FLOAT dt);

		void AddForce(const DirectX::XMFLOAT3& _force);

	protected:
		DirectX::XMFLOAT3 mPosition;
		DirectX::XMFLOAT3 mVelocity;
		DirectX::XMFLOAT3 mAcceleration;

		FLOAT mDamping;
		FLOAT mInverseMass;

		DirectX::XMFLOAT3 mForceAccum;
	};
}

#include "Particle.inl"