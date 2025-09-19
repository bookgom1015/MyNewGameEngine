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
				const std::string& name,
				const DirectX::XMFLOAT3& pos = { 0.f, 0.f, 0.f },
				const DirectX::XMFLOAT4& rot = { 0.f, 0.f, 0.f, 1.f },
				const DirectX::XMFLOAT3& scale = { 1.f, 1.f, 1.f });
			FreeLookActor(
				Common::Debug::LogFile* const pLogFile, 
				const std::string& name, 
				const Common::Foundation::Mesh::Transform& trans);
			virtual ~FreeLookActor() = default;

		public:
			__forceinline const DirectX::XMVECTOR& CameraForwardVector() const;
			__forceinline const DirectX::XMVECTOR& CameraRightVector() const;
			__forceinline const DirectX::XMVECTOR& CameraUpVector() const;

			__forceinline DirectX::XMVECTOR CameraRotation() const;

		public:
			virtual BOOL ProcessActorInput(Common::Input::InputState* const pInput) override;
			virtual BOOL UpdateActor(FLOAT delta) override;

		public:
			Foundation::Camera::CameraComponent* mpCameraComp = nullptr;

			FLOAT mForwardSpeed = 0.f;
			FLOAT mStrapeSpeed = 0.f;

			FLOAT mWalkSpeed = 4.f;
			FLOAT mActualWalkSpeed = 4.f;

			FLOAT mLookUpSpeed = 0.f;
			FLOAT mTurnSpeed = 0.f;

			FLOAT mLookSensitivity = 0.004f;
			FLOAT mTurnSensitivity = 0.004f;

			DirectX::XMFLOAT2 mPrevMousePos = { 0.f, 0.f };
		};
	}
}