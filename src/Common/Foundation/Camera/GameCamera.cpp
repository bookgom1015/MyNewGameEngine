#include "Common/Foundation/Camera/GameCamera.hpp"
#include "Common/Foundation/Core/WindowsManager.hpp"

using namespace Common::Foundation::Camera;
using namespace DirectX;

GameCamera::GameCamera(Core::WindowsManager* const pWndManager, FLOAT nearZ, FLOAT farZ, FLOAT fovY) {
	mpWndManager = pWndManager;
	mNearZ = nearZ;
	mFarZ = farZ;
	mFovY = fovY;
}

XMVECTOR GameCamera::Rotation() const {
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

void GameCamera::Pitch(FLOAT rad) {
	const auto quat = XMQuaternionRotationAxis(mRight, rad);

	mUp = XMVector3Rotate(mUp, quat);
	mForward = XMVector3Rotate(mForward, quat);

	mbViewDirty = TRUE;
}

void GameCamera::Yaw(FLOAT rad) {
	const auto quat = XMQuaternionRotationAxis(UnitVector::UpVector, rad);

	mRight = XMVector3Rotate(mRight, quat);
	mUp = XMVector3Rotate(mUp, quat);
	mForward = XMVector3Rotate(mForward, quat);

	mbViewDirty = TRUE;
}

void GameCamera::Roll(FLOAT rad) {
	const auto quat = XMQuaternionRotationAxis(mForward, rad);

	mRight = XMVector3Rotate(mRight, quat);
	mUp = XMVector3Rotate(mUp, quat);

	mbViewDirty = TRUE;
}

void GameCamera::AddPosition(const XMVECTOR& pos) {
	mPosition += pos;

	mbViewDirty = TRUE;
}

void GameCamera::SetPosition(const XMVECTOR& pos) {
	mPosition = pos;

	mbViewDirty = TRUE;
}