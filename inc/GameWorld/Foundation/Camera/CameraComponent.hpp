#pragma once

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
			virtual ~CameraComponent();

		public:
			virtual bool OnInitialzing() override;
			virtual void OnCleaningUp() override;

			virtual bool ProcessInput(Common::Input::InputState* const pInput) override;
			virtual bool Update(float delta) override;
			virtual bool OnUpdateWorldTransform() override;

		public:
			DirectX::SimpleMath::Vector3 Position() const;
			DirectX::SimpleMath::Vector3 Rotation() const;

			DirectX::SimpleMath::Matrix View() const;

			DirectX::SimpleMath::Vector3 RightVector() const;
			DirectX::SimpleMath::Vector3 UpVector() const;
			DirectX::SimpleMath::Vector3 ForwardVector() const;

		public: // Controlling transform functions
			void Pitch(float degree);
			void Yaw(float degree);
			void Roll(float degree);

			void AddPosition(const DirectX::SimpleMath::Vector3& pos);
			void SetPosition(const DirectX::SimpleMath::Vector3& pos);

		private:
			std::unique_ptr<Common::Foundation::Camera::GameCamera> mCamera{};

			float mPitch{};
			float mYaw{};
			float mRoll{};

			bool mbLimitPitch{ true };
			bool mbLimitYaw{};
			bool mbLimitRoll{};

			float mPitchLimit{ 85.f };
			float mYawLimit{};
			float mRollLimit{};
		};
	}
}