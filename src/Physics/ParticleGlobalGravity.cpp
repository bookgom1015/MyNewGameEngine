#include "Physics/ParticleGlobalGravity.hpp"
#include "Physics/Particle.hpp"

using namespace Physics::Cyclone;
using namespace DirectX;

void ParticleGlobalGravity::UpdateForce(Particle* pParticle, float  dt) {
	if (!pParticle->HasFiniteMass()) return;
	pParticle->AddForce(mGravity * pParticle->GetMass());
}