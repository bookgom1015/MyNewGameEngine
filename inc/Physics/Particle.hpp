#pragma once

#include <algorithm>

#include <DirectXMath.h>

namespace Physics::Cyclone {
	class Particle {
	public:
		Particle() = default;
		virtual ~Particle() = default;

	public:
		__forceinline constexpr float GetMass() const noexcept;
		__forceinline constexpr void SetMass(float mass) noexcept;

		__forceinline constexpr void SetInverseMass(float invMass) noexcept;

		__forceinline constexpr bool HasFiniteMass() const;

		__forceinline DirectX::XMVECTOR& GetVelocity();
		__forceinline const DirectX::XMVECTOR& GetVelocity() const;

		__forceinline DirectX::XMVECTOR& GetPosition();
		__forceinline const DirectX::XMVECTOR& GetPosition() const;

	public:
		void Integrate(float dt);

		void AddForce(const DirectX::XMFLOAT3& force);
		void AddForce(const DirectX::XMVECTOR& force);

	protected:
		DirectX::XMVECTOR mPosition{};
		DirectX::XMVECTOR mVelocity{};
		DirectX::XMVECTOR mAcceleration{};

		float mDamping{};
		float mInverseMass{};

		DirectX::XMVECTOR mForceAccum{};
	};
}

#include "Particle.inl"