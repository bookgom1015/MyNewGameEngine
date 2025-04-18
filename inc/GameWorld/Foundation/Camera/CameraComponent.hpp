#pragma once

#include <memory>

#include <DirectXMath.h>

#include "GameWorld/Foundation/Core/Component.hpp"

namespace Common::Foundation::Camera {
	class GameCamera;
}

namespace GameWorld::Foundation {
	namespace Core {
		class Actor;
	}

	namespace Camera {
		class CameraComponent : public Core::Component {
		public:
			CameraComponent(Common::Debug::LogFile* const pLogFile, Core::Actor* const pOwner);
			virtual ~CameraComponent() = default;

		public:
			virtual BOOL OnInitialzing() override;
			virtual void OnCleaningUp() override;

			virtual BOOL ProcessInput(Common::Input::InputState* const pInput) override;
			virtual BOOL Update(FLOAT delta) override;
			virtual BOOL OnUpdateWorldTransform() override;

		public:
			DirectX::XMVECTOR Position() const;
			DirectX::XMVECTOR Rotation() const;

			DirectX::XMFLOAT4X4 View() const;

			DirectX::XMVECTOR RightVector() const;
			DirectX::XMVECTOR UpVector() const;
			DirectX::XMVECTOR ForwardVector() const;

		public: // Controlling transform functions
			void Pitch(FLOAT rad);
			void Yaw(FLOAT rad);
			void Roll(FLOAT rad);

			void AddPosition(const DirectX::XMVECTOR& pos);
			void SetPosition(const DirectX::XMVECTOR& pos);

		private:
			std::unique_ptr<Common::Foundation::Camera::GameCamera> mCamera;

			FLOAT mPitch = 0.f;
			FLOAT mYaw = 0.f;
			FLOAT mRoll = 0.f;

			BOOL mbLimitPitch = TRUE;
			BOOL mbLimitYaw = FALSE;
			BOOL mbLimitRoll = FALSE;

			FLOAT mPitchLimit = DirectX::XM_PIDIV2 - 0.1f;
			FLOAT mYawLimit = 0.f;
			FLOAT mRollLimit = 0.f;
		};
	}
}