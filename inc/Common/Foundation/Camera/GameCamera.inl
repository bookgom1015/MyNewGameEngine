#ifndef __GAMECAMERA_INL__
#define __GAMECAMERA_INL__

FLOAT Common::Foundation::Camera::GameCamera::FovY() const { return mFovY; }

FLOAT Common::Foundation::Camera::GameCamera::NearZ() const { return mNearZ; }

FLOAT Common::Foundation::Camera::GameCamera::FarZ() const { return mFarZ; }

DirectX::XMVECTOR Common::Foundation::Camera::GameCamera::RightVector() const { return mRight; }

DirectX::XMVECTOR Common::Foundation::Camera::GameCamera::UpVector() const { return mUp; }

DirectX::XMVECTOR Common::Foundation::Camera::GameCamera::ForwardVector() const { return mForward; }

DirectX::XMFLOAT4X4 Common::Foundation::Camera::GameCamera::GameCamera::View() const { return mView; }

DirectX::XMFLOAT4X4 Common::Foundation::Camera::GameCamera::GameCamera::Proj() const { return mProj; }

DirectX::XMVECTOR Common::Foundation::Camera::GameCamera::GameCamera::Position() const { return mPosition; }

#endif // __GAMECAMERA_INL__