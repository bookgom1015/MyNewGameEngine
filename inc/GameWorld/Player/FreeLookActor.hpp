#pragma once

#include "GameWorld/Foundation/Core/Actor.hpp"

namespace GameWorld {
	namespace Foundation::Camera {
		class CameraComponent;
	}

	namespace Player {
		class FreeLookActor : public GameWorld::Foundation::Core::Actor {
		public:
			FreeLookActor(
				Common::Debug::LogFile* const pLogFile,
				const std::string& name);
			FreeLookActor(
				Common::Debug::LogFile* const pLogFile, 
				const std::string& name, 
				const Common::Foundation::Mesh::Transform& trans);
			virtual ~FreeLookActor();

		public:
			__forceinline const DirectX::SimpleMath::Vector3& CameraForwardVector() const;
			__forceinline const DirectX::SimpleMath::Vector3& CameraRightVector() const;
			__forceinline const DirectX::SimpleMath::Vector3& CameraUpVector() const;

			__forceinline DirectX::SimpleMath::Vector3 CameraRotation() const;

		public:
			virtual bool ProcessActorInput(Common::Input::InputState* const pInputState) override;
			virtual bool UpdateActor(float delta)override;

		public:
			Foundation::Camera::CameraComponent* mpCameraComp{};

			float mForwardSpeed{};
			float mStrapeSpeed{};

			float mWalkSpeed{ 4.f };
			float mActualWalkSpeed{ 4.f };

			float mLookUpSpeed{};
			float mTurnSpeed{};

			float mLookSensitivity{ 0.25f };
			float mTurnSensitivity{ 0.25f };

			DirectX::SimpleMath::Vector2 mPrevMousePos{};
		};
	}
}