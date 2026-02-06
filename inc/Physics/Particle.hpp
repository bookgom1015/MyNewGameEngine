/*
 * Interface file for the particle class.
 *
 * Part of the Cyclone physics system.
 *
 * Copyright (c) Icosagon 2003. All Rights Reserved.
 *
 * This software is distributed under licence. Use of this software
 * implies agreement with all terms and conditions of the accompanying
 * software licence.
 */

#pragma once

namespace Physics::Cyclone {
	class Particle {
	public:
		Particle() = default;
		virtual ~Particle() = default;

	public:
		__forceinline constexpr float GetMass() const noexcept;
		__forceinline constexpr void SetMass(float mass) noexcept;

		__forceinline constexpr float GetInverseMass() const noexcept;
		__forceinline constexpr void SetInverseMass(float invMass) noexcept;

		__forceinline constexpr bool HasFiniteMass() const;

		__forceinline void SetDamping(float damping) noexcept;

		__forceinline void SetAcceleration(const DirectX::SimpleMath::Vector3& accel);
		__forceinline DirectX::SimpleMath::Vector3& GetAcceleration();
		__forceinline const DirectX::SimpleMath::Vector3& GetAcceleration() const;

		__forceinline void SetVelocity(const DirectX::SimpleMath::Vector3& vel);
		__forceinline DirectX::SimpleMath::Vector3& GetVelocity();
		__forceinline const DirectX::SimpleMath::Vector3& GetVelocity() const;

		__forceinline void SetPosition(const DirectX::SimpleMath::Vector3& pos);
		__forceinline DirectX::SimpleMath::Vector3& GetPosition();
		__forceinline const DirectX::SimpleMath::Vector3& GetPosition() const;

		// Clears the forces applied to the particle. This will be
		// called automatically after each integration step.
		__forceinline void ClearAccumulator() noexcept;

	public:
		// Integrates the particle forward in time by the given amount.
		// This function uses a Newton-Euler integration method, which is a
		// linear approximation to the correct integral. For this reason it
		// may be inaccurate in some cases.
		void Integrate(float dt);

		//Adds the given force to the particle, to be applied at the
		//next iteration only.
		void AddForce(const DirectX::SimpleMath::Vector3& force);

	protected:
		// Holds the linear position of the particle in world space.
		DirectX::SimpleMath::Vector3 mPosition{};
		// Holds the linear velocity of the particle in world space.
		DirectX::SimpleMath::Vector3 mVelocity{};
		// Holds the acceleration of the particle.  This value
		// can be used to set acceleration due to gravity (its primary
		// use), or any other constant acceleration.
		DirectX::SimpleMath::Vector3 mAcceleration{};

		// Holds the amount of damping applied to linear
		// motion. Damping is required to remove energy added
		// through numerical instability in the integrator.
		float mDamping{};
		// Holds the inverse of the mass of the particle. It
		// is more useful to hold the inverse mass because
		// integration is simpler, and because in real time
		// simulation it is more useful to have objects with
		// infinite mass (immovable) than zero mass
		// (completely unstable in numerical simulation).
		float mInverseMass{};

		// Holds the accumulated force to be applied at the next
		// simulation iteration only. This value is zeroed at each
		// integration step.
		DirectX::SimpleMath::Vector3 mForceAccum{};
	};
}

#include "Particle.inl"