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

			__forceinline DirectX::XMVECTOR RightVector() const;
			__forceinline DirectX::XMVECTOR UpVector() const;
			__forceinline DirectX::XMVECTOR ForwardVector() const;

			__forceinline DirectX::XMFLOAT4X4 View() const;
			__forceinline DirectX::XMFLOAT4X4 Proj() const;

			__forceinline DirectX::XMVECTOR Position() const;

		public:
			DirectX::XMVECTOR Rotation() const;

		public:
			void UpdateViewMatrix();

			void Pitch(float rad);
			void Yaw(float rad);
			void Roll(float rad);

			void AddPosition(const DirectX::XMVECTOR& pos);
			void SetPosition(const DirectX::XMVECTOR& pos);

		private:
			Core::WindowsManager* mpWndManager{};

			DirectX::XMVECTOR mPosition = DirectX::XMVectorZero();
			DirectX::XMVECTOR mRight = UnitVector::RightVector;
			DirectX::XMVECTOR mUp = UnitVector::UpVector;
			DirectX::XMVECTOR mForward = UnitVector::ForwardVector;

			float mNearZ{};
			float mFarZ{};
			float mFovY{};

			bool mbViewDirty{ TRUE };

			DirectX::XMFLOAT4X4 mView = Util::MathUtil::Identity4x4();
			DirectX::XMFLOAT4X4 mProj = Util::MathUtil::Identity4x4();
		};
	}
}

#include "Common/Foundation/Camera/GameCamera.inl"