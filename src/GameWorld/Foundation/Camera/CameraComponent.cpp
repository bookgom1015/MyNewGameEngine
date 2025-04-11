#include "GameWorld/Foundation/Camera/CameraComponent.hpp"
#include "Common/Foundation/Camera/GameCamera.hpp"
#include "Common/Render/Renderer.hpp"
#include "GameWorld/GameWorld.hpp"

using namespace GameWorld::Foundation::Camera;
using namespace DirectX;

CameraComponent::CameraComponent(Core::Actor* const pOwner)
	: Component(pOwner) {
	mCamera = std::make_unique<Common::Foundation::Camera::GameCamera>(GameWorld::GameWorldClass::spGameWorld->WindowsManager());
}

BOOL CameraComponent::OnInitialzing() {
	GameWorldClass::spGameWorld->Renderer()->SetCamera(mCamera.get());
	
	return TRUE;
}

BOOL CameraComponent::ProcessInput(Common::Input::InputState* const pInput) { return TRUE; }

BOOL CameraComponent::Update(FLOAT delta) { return TRUE; }

BOOL CameraComponent::OnUpdateWorldTransform() {
	mCamera->SetPosition(ActorTransform().Position);
	mCamera->UpdateViewMatrix();

	return TRUE;
}

XMVECTOR CameraComponent::Position() const { return mCamera->Position(); }

XMVECTOR CameraComponent::Rotation() const { return mCamera->Rotation(); }

XMFLOAT4X4 CameraComponent::View() const { return mCamera->View(); }

XMVECTOR CameraComponent::RightVector() const { return mCamera->RightVector(); }

XMVECTOR CameraComponent::UpVector() const { return mCamera->UpVector(); }

XMVECTOR CameraComponent::ForwardVector() const { return mCamera->ForwardVector(); }

void CameraComponent::Pitch(FLOAT rad) {
	if (mbLimitPitch) {
		const FLOAT newPitch = mPitch + rad;
		if (newPitch >= mPitchLimit) rad = mPitchLimit - mPitch;
		else if (newPitch <= -mPitchLimit) rad = -mPitchLimit - mPitch;
		mPitch += rad;
	}
	else {
		mPitch += rad;
		if (mPitch >= XM_2PI) mPitch = 0.f;
		else if (mPitch <= -XM_2PI) mPitch = 0.f;
	}
	mCamera->Pitch(rad);
}

void CameraComponent::Yaw(FLOAT rad) {
	mCamera->Yaw(rad);
}

void CameraComponent::Roll(FLOAT rad) {
	mCamera->Roll(rad);
}

void CameraComponent::AddPosition(const XMVECTOR& pos) {
	mCamera->AddPosition(pos);
}

void CameraComponent::SetPosition(const XMVECTOR& pos) {
	mCamera->SetPosition(pos);
}