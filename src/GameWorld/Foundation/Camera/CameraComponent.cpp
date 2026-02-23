#include "GameWorld/Foundation/Core/pch_world.h"
#include "GameWorld/Foundation/Camera/CameraComponent.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Camera/GameCamera.hpp"
#include "Common/Render/Renderer.hpp"
#include "GameWorld/GameWorld.hpp"

using namespace GameWorld::Foundation::Camera;
using namespace DirectX;
using namespace DirectX::SimpleMath;

CameraComponent::CameraComponent(Common::Debug::LogFile* const pLogFile, Core::Actor* const pOwner)
	: Component(pLogFile, pOwner) {
	mCamera = std::make_unique<Common::Foundation::Camera::GameCamera>(GameWorld::GameWorldClass::spGameWorld->WindowsManager());
}

CameraComponent::~CameraComponent() {}

bool CameraComponent::OnInitialzing() {
	GameWorldClass::spGameWorld->Renderer()->SetCamera(mCamera.get());
	
	return true;
}

void CameraComponent::OnCleaningUp() {}

bool CameraComponent::ProcessInput(Common::Input::InputState* const pInput) { return true; }

bool CameraComponent::Update(FLOAT delta) { return true; }

bool CameraComponent::OnUpdateWorldTransform() {
	mCamera->SetPosition(ActorTransform().Position);
	mCamera->UpdateViewMatrix();

	return true;
}

Vector3 CameraComponent::Position() const { return mCamera->Position(); }

Vector3 CameraComponent::Rotation() const { return mCamera->Rotation(); }

Matrix CameraComponent::View() const { return mCamera->View(); }

Vector3 CameraComponent::RightVector() const { return mCamera->RightVector(); }

Vector3 CameraComponent::UpVector() const { return mCamera->UpVector(); }

Vector3 CameraComponent::ForwardVector() const { return mCamera->ForwardVector(); }

void CameraComponent::Pitch(float degree) {
	if (mbLimitPitch) {
		const FLOAT newPitch = mPitch + degree;
		if (newPitch >= mPitchLimit) degree = mPitchLimit - mPitch;
		else if (newPitch <= -mPitchLimit) degree = -mPitchLimit - mPitch;
		mPitch += degree;
	}
	else {
		mPitch += degree;
		if (mPitch >= XM_2PI) mPitch = 0.f;
		else if (mPitch <= -XM_2PI) mPitch = 0.f;
	}
	mCamera->Pitch(degree);
}

void CameraComponent::Yaw(float degree) {
	mCamera->Yaw(degree);
}

void CameraComponent::Roll(float degree) {
	mCamera->Roll(degree);
}

void CameraComponent::AddPosition(const Vector3& pos) {
	mCamera->AddPosition(pos);
}

void CameraComponent::SetPosition(const Vector3& pos) {
	mCamera->SetPosition(pos);
}