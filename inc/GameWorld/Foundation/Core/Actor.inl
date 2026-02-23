#ifndef __ACTOR_INL__
#define __ACTOR_INL__

namespace GameWorld::Foundation::Core {
	constexpr const std::string& Actor::Name() const { return mName; }

	constexpr const Common::Foundation::Mesh::Transform& Actor::GetTransform() const { return mTransform; }

	constexpr bool Actor::Initialized() const { return mbInitialized; }

	constexpr bool Actor::IsDead() const { return mbIsDead; }

	void Actor::SetPosition(const DirectX::SimpleMath::Vector3& pos) {
		mbNeedToUpdate = TRUE;
		mTransform.Position = pos;
	}

	void Actor::AddPosition(const DirectX::SimpleMath::Vector3& pos) {
		mbNeedToUpdate = TRUE;
		mTransform.Position += pos;
	}

	void Actor::SetRotation(const DirectX::SimpleMath::Vector3& rot) {
		mbNeedToUpdate = TRUE;
		mTransform.Rotation = rot;
	}

	void Actor::AddRotation(const DirectX::SimpleMath::Vector3& rot) {
		mbNeedToUpdate = TRUE;
		mTransform.Rotation += rot;
	}

	void Actor::AddRotationPitch(float degree) {
		mbNeedToUpdate = TRUE;
		mTransform.Rotation = DirectX::SimpleMath::Vector3(degree, 0.f, 0.f);
	}

	void Actor::AddRotationYaw(float degree) {
		mbNeedToUpdate = TRUE;
		mTransform.Rotation = DirectX::SimpleMath::Vector3(0.f, degree, 0.f);
	}

	void Actor::AddRotationRoll(float degree) {
		mbNeedToUpdate = TRUE;
		mTransform.Rotation = DirectX::SimpleMath::Vector3(0.f, 0.f, degree);
	}

	void Actor::SetScale(const DirectX::SimpleMath::Vector3& scale) {
		mbNeedToUpdate = TRUE;
		mTransform.Scale = scale;
	}
}

#endif // __ACTOR_INL__