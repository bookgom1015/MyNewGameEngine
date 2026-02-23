#include "GameWorld/Foundation/Core/pch_world.h"
#include "GameWorld/Player/FreeLookActor.hpp"
#include "Common/Input/InputProcessor.hpp"
#include "GameWorld/Foundation/Camera/CameraComponent.hpp"

using namespace GameWorld::Player;
using namespace DirectX;
using namespace DirectX::SimpleMath;

FreeLookActor::FreeLookActor(
		Common::Debug::LogFile* const pLogFile, 
		const std::string& name) 
	: Actor(pLogFile, name) {
	mpCameraComp = new GameWorld::Foundation::Camera::CameraComponent(pLogFile, this);
}

FreeLookActor::FreeLookActor(
		Common::Debug::LogFile* const pLogFile, 
		const std::string& name, 
		const Common::Foundation::Mesh::Transform& trans)
	: Actor(pLogFile, name, trans) {
	mpCameraComp = new GameWorld::Foundation::Camera::CameraComponent(pLogFile, this);
}

FreeLookActor::~FreeLookActor() {}

const Vector3& FreeLookActor::CameraForwardVector() const { return mpCameraComp->ForwardVector(); }

const Vector3& FreeLookActor::CameraRightVector() const { return mpCameraComp->RightVector(); }

const Vector3& FreeLookActor::CameraUpVector() const {	return mpCameraComp->UpVector(); }

Vector3 FreeLookActor::CameraRotation() const { return mpCameraComp->Rotation(); }

bool FreeLookActor::ProcessActorInput(Common::Input::InputState* const pInput) {
	mForwardSpeed = 0.f;
	mStrapeSpeed = 0.f;

	if (pInput->Keyboard.KeyValue(VK_LSHIFT))
		mActualWalkSpeed = mWalkSpeed * 4.f;
	else if (pInput->Keyboard.KeyValue(VK_LCONTROL))
		mActualWalkSpeed = mWalkSpeed * 0.1f;
	else
		mActualWalkSpeed = mWalkSpeed;

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

bool FreeLookActor::UpdateActor(float delta) {
	Vector3 strape = mpCameraComp->RightVector() * mStrapeSpeed;
	Vector3 forward = mpCameraComp->ForwardVector() * mForwardSpeed;

	Vector3 direction = strape + forward;
	direction.Normalize();

	float yaw = mTurnSpeed * mLookSensitivity;
	float pitch = mLookUpSpeed * mTurnSensitivity;

	AddPosition(direction * mActualWalkSpeed * delta);
	AddRotationYaw(yaw);

	mpCameraComp->Yaw(yaw);
	mpCameraComp->Pitch(pitch);

	return true;
}