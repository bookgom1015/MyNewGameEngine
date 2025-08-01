#include "GameWorld/Player/FreeLookActor.hpp"
#include "Common/Input/InputProcessor.hpp"
#include "GameWorld/Foundation/Camera/CameraComponent.hpp"

using namespace GameWorld::Player;
using namespace DirectX;

FreeLookActor::FreeLookActor(
		Common::Debug::LogFile* const pLogFile, 
		const std::string& name, 
		const XMFLOAT3& pos, 
		const XMFLOAT4& rot, 
		const XMFLOAT3& scale) 
	: Actor(pLogFile, name, pos, rot, scale) {
	mpCameraComp = new GameWorld::Foundation::Camera::CameraComponent(pLogFile, this);
}

FreeLookActor::FreeLookActor(
		Common::Debug::LogFile* const pLogFile, 
		const std::string& name, 
		const Common::Foundation::Mesh::Transform& trans)
	: Actor(pLogFile, name, trans) {
	mpCameraComp = new GameWorld::Foundation::Camera::CameraComponent(pLogFile, this);
}

const XMVECTOR& FreeLookActor::CameraForwardVector() const { return mpCameraComp->ForwardVector(); }

const XMVECTOR& FreeLookActor::CameraRightVector() const { return mpCameraComp->RightVector(); }

const XMVECTOR& FreeLookActor::CameraUpVector() const {	return mpCameraComp->UpVector(); }

XMVECTOR FreeLookActor::CameraRotation() const { return mpCameraComp->Rotation(); }

BOOL FreeLookActor::ProcessActorInput(Common::Input::InputState* const pInput) {
	mForwardSpeed = 0.f;
	mStrapeSpeed = 0.f;

	mActualWalkSpeed = pInput->Keyboard.KeyValue(VK_LSHIFT) ? mWalkSpeed * 0.1f : mWalkSpeed;

	if (pInput->Keyboard.KeyValue(VK_W)) mForwardSpeed += 1.f;
	if (pInput->Keyboard.KeyValue(VK_S)) mForwardSpeed += -1.f;
	if (pInput->Keyboard.KeyValue(VK_A)) mStrapeSpeed += -1.f;
	if (pInput->Keyboard.KeyValue(VK_D)) mStrapeSpeed += 1.f;

	mLookUpSpeed = 0.f;
	mTurnSpeed = 0.f;

	if (pInput->Mouse.ButtonState(VK_RBUTTON) == Common::Input::ButtonStates::E_Pressed) {
		const auto CurrMousePos = pInput->Mouse.MousePosition();
		const auto CurrMousePosV = XMLoadFloat2(&CurrMousePos);
		const auto PrevMousePosV = XMLoadFloat2(&mPrevMousePos);

		const auto Displacement = CurrMousePosV - PrevMousePosV;

		mLookUpSpeed = Displacement.m128_f32[1];
		mTurnSpeed = Displacement.m128_f32[0];

		XMStoreFloat2(&mPrevMousePos, CurrMousePosV);
	}
	else {
		mPrevMousePos = pInput->Mouse.MousePosition();
	}

	return TRUE;
}

BOOL FreeLookActor::UpdateActor(FLOAT delta) {
	const XMVECTOR Strape = mpCameraComp->RightVector() * mStrapeSpeed;
	const XMVECTOR Forward = mpCameraComp->ForwardVector() * mForwardSpeed;
	const XMVECTOR Direction = Strape + Forward;
	const XMVECTOR Normalized = XMVector3Normalize(Direction);

	FLOAT yaw = mTurnSpeed * mLookSensitivity;
	FLOAT pitch = mLookUpSpeed * mTurnSensitivity;

	AddPosition(Normalized * mActualWalkSpeed * delta);
	AddRotationYaw(yaw);

	mpCameraComp->Yaw(yaw);
	mpCameraComp->Pitch(pitch);

	return TRUE;
}