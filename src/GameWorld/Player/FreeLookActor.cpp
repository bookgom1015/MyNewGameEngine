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

	float speed = 1.f;
	if (pInput->Keyboard.KeyValue(VK_LSHIFT)) speed *= 0.1f;

	if (pInput->Keyboard.KeyValue(VK_W)) mForwardSpeed += speed;
	if (pInput->Keyboard.KeyValue(VK_S)) mForwardSpeed += -speed;
	if (pInput->Keyboard.KeyValue(VK_A)) mStrapeSpeed += -speed;
	if (pInput->Keyboard.KeyValue(VK_D)) mStrapeSpeed += speed;

	mLookUpSpeed = 0.f;
	mTurnSpeed = 0.f;

	if (pInput->Mouse.ButtonState(VK_LBUTTON) == Common::Input::ButtonStates::E_Pressed) {
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
	XMVECTOR strape = mpCameraComp->RightVector() * mStrapeSpeed;
	XMVECTOR forward = mpCameraComp->ForwardVector() * mForwardSpeed;
	XMVECTOR disp = strape + forward;

	FLOAT yaw = mTurnSpeed * mLookSensitivity;
	FLOAT pitch = mLookUpSpeed * mTurnSensitivity;

	AddPosition(disp * mWalkSpeed * delta);
	AddRotationYaw(yaw);

	mpCameraComp->Yaw(yaw);
	mpCameraComp->Pitch(pitch);

	return TRUE;
}