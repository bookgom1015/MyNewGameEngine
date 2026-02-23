#ifndef __GAMECAMERA_INL__
#define __GAMECAMERA_INL__

float Common::Foundation::Camera::GameCamera::FovY() const { return mFovY; }

float Common::Foundation::Camera::GameCamera::NearZ() const { return mNearZ; }

float Common::Foundation::Camera::GameCamera::FarZ() const { return mFarZ; }

DirectX::SimpleMath::Vector3 Common::Foundation::Camera::GameCamera::RightVector() const { return mRight; }

DirectX::SimpleMath::Vector3 Common::Foundation::Camera::GameCamera::UpVector() const { return mUp; }

DirectX::SimpleMath::Vector3 Common::Foundation::Camera::GameCamera::ForwardVector() const { return mForward; }

DirectX::SimpleMath::Matrix Common::Foundation::Camera::GameCamera::GameCamera::View() const { return mView; }

DirectX::SimpleMath::Matrix Common::Foundation::Camera::GameCamera::GameCamera::Proj() const { return mProj; }

DirectX::SimpleMath::Vector3 Common::Foundation::Camera::GameCamera::GameCamera::Position() const { return mPosition; }

#endif // __GAMECAMERA_INL__