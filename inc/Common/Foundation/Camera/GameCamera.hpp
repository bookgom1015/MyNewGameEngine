#pragma once

#include "Common/Util/MathUtil.hpp"

namespace Common::Foundation {
	namespace Core {
		class WindowsManager;
	}

	namespace Camera {
		class GameCamera {
		public:
			GameCamera(
				Core::WindowsManager* const pWndManager,
				float nearZ = 0.1f,
				float farZ = 1000.f,
				float fovY = 90.f);
			virtual ~GameCamera();

		public:
			__forceinline float FovY() const;
			__forceinline float NearZ() const;
			__forceinline float FarZ() const;

			__forceinline DirectX::SimpleMath::Vector3 RightVector() const;
			__forceinline DirectX::SimpleMath::Vector3 UpVector() const;
			__forceinline DirectX::SimpleMath::Vector3 ForwardVector() const;

			__forceinline DirectX::SimpleMath::Matrix View() const;
			__forceinline DirectX::SimpleMath::Matrix Proj() const;

			__forceinline DirectX::SimpleMath::Vector3 Position() const;

		public:
			DirectX::SimpleMath::Vector3 Rotation() const;

		public:
			void UpdateViewMatrix();

			void Pitch(float degree);
			void Yaw(float degree);
			void Roll(float degree);

			void AddPosition(const DirectX::SimpleMath::Vector3& pos);
			void SetPosition(const DirectX::SimpleMath::Vector3& pos);

		private:
			Core::WindowsManager* mpWndManager{};

			DirectX::SimpleMath::Vector3 mPosition = UnitVector::ZeroVector;
			DirectX::SimpleMath::Vector3 mRight = UnitVector::RightVector;
			DirectX::SimpleMath::Vector3 mUp = UnitVector::UpVector;
			DirectX::SimpleMath::Vector3 mForward = UnitVector::ForwardVector;

			float mNearZ{};
			float mFarZ{};
			float mFovY{};

			bool mbViewDirty{ true };

			DirectX::SimpleMath::Matrix mView = DirectX::SimpleMath::Matrix::Identity;
			DirectX::SimpleMath::Matrix mProj = DirectX::SimpleMath::Matrix::Identity;
		};
	}
}

#include "Common/Foundation/Camera/GameCamera.inl"