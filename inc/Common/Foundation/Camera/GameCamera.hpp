#pragma once

#include "Common/Foundation/Util/MathUtil.hpp"

namespace Common::Foundation {
	namespace Core {
		class WindowsManager;
	}

	namespace Camera {
		class GameCamera {
		public:
			GameCamera(Core::WindowsManager* const pWndManager, FLOAT nearZ, FLOAT farZ, FLOAT fovY);
			virtual ~GameCamera() = default;

		public:
			__forceinline FLOAT FovY() const;
			__forceinline FLOAT NearZ() const;
			__forceinline FLOAT FarZ() const;

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

			void Pitch(FLOAT rad);
			void Yaw(FLOAT rad);
			void Roll(FLOAT rad);

			void AddPosition(const DirectX::XMVECTOR& pos);
			void SetPosition(const DirectX::XMVECTOR& pos);

		private:
			Core::WindowsManager* mpWndManager = nullptr;

			DirectX::XMVECTOR mPosition = DirectX::XMVectorZero();
			DirectX::XMVECTOR mRight = UnitVector::RightVector;
			DirectX::XMVECTOR mUp = UnitVector::UpVector;
			DirectX::XMVECTOR mForward = UnitVector::ForwardVector;

			FLOAT mNearZ;
			FLOAT mFarZ;
			FLOAT mFovY;

			BOOL mbViewDirty = TRUE;

			DirectX::XMFLOAT4X4 mView = Util::MathUtil::Identity4x4();
			DirectX::XMFLOAT4X4 mProj = Util::MathUtil::Identity4x4();
		};
	}
}

#include "Common/Foundation/Camera/GameCamera.inl"