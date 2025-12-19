#ifndef __PARTICLE_INL__
#define __PARTICLE_INL__

constexpr void Physics::Cyclone::Particle::SetMass(FLOAT mass) {
	mInverseMass = 1.f / mass;
}

constexpr void Physics::Cyclone::Particle::SetInverseMass(FLOAT invMass) {
	mInverseMass = invMass;
}

#endif // __PARTICLE_INL__