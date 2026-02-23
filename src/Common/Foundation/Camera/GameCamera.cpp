#include "Common/Foundation/Camera/GameCamera.hpp"
#include "Common/Foundation/Core/WindowsManager.hpp"

using namespace Common::Foundation::Camera;
using namespace DirectX;
using namespace DirectX::SimpleMath;

GameCamera::GameCamera(
		Core::WindowsManager* const pWndManager, 
		float nearZ, float farZ, float fovY) {
	mpWndManager = pWndManager;
	mNearZ = nearZ;
	mFarZ = farZ;
	mFovY = fovY;
}

GameCamera::~GameCamera() {}

Vector3 GameCamera::Rotation() const {
	return XMQuaternionRotationMatrix(XMMatrixLookAtLH(
		XMVectorZero(),
		UnitVector::ForwardVector,
		mUp
	));
}

void GameCamera::UpdateViewMatrix() {
	if (!mbViewDirty) return;
	mbViewDirty = FALSE;

	XMStoreFloat4x4(
		&mView,
		XMMatrixLookAtLH(
			mPosition,
			mPosition + mForward,
			mUp
		)
	);

	XMStoreFloat4x4(
		&mProj,
		XMMatrixPerspectiveFovLH(
			XM_PIDIV2,
			mpWndManager->AspectRatio(),
			mNearZ,
			mFarZ
		)
	);
}

void GameCamera::Pitch(float degree) {
	const float rad = degree * DegToRad;
	const auto quat = XMQuaternionRotationAxis(mRight, rad);

	mUp = XMVector3Rotate(mUp, quat);
	mForward = XMVector3Rotate(mForward, quat);

	mbViewDirty = true;
}

void GameCamera::Yaw(float degree) {
	const float rad = degree * DegToRad;
	const auto quat = XMQuaternionRotationAxis(UnitVector::UpVector, rad);

	mRight = XMVector3Rotate(mRight, quat);
	mUp = XMVector3Rotate(mUp, quat);
	mForward = XMVector3Rotate(mForward, quat);

	mbViewDirty = true;
}

void GameCamera::Roll(float degree) {
	const float rad = degree * DegToRad;
	const auto quat = XMQuaternionRotationAxis(mForward, rad);

	mRight = XMVector3Rotate(mRight, quat);
	mUp = XMVector3Rotate(mUp, quat);

	mbViewDirty = true;
}

void GameCamera::AddPosition(const Vector3& pos) {
	mPosition += pos;

	mbViewDirty = true;
}

void GameCamera::SetPosition(const Vector3& pos) {
	mPosition = pos;

	mbViewDirty = true;
}