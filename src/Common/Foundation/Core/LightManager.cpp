#include "Common/Foundation/Core/LightManager.hpp"
#include "Common/Debug/Logger.hpp"

#include <SimpleMath.h>

using namespace Common::Foundation::Core;

using namespace DirectX::SimpleMath;

namespace {
	const std::wstring ERR_NUM_LIGHTS_LIMIT = L"Can not add light due to the light count limit";
}

LightManager::LightManager() {}

LightManager::~LightManager() { CleanUp(); }

bool LightManager::Initialize(Debug::LogFile* const pLogFile) {
	mpLogFile = pLogFile;

	return true;
}

void LightManager::CleanUp() {
	if (mpCleanedUp) return;

	mpCleanedUp = true;
}

bool LightManager::AddLight(const Light& light) {
	if (mLightCount >= MaxLights) ReturnFalse(mpLogFile, ERR_NUM_LIGHTS_LIMIT);
		
	auto newLight = std::make_unique<Light>(light);
	mLights[mLightCount++] = std::move(newLight);

	return true;
}

bool LightManager::AddLight(LightType type) {
	if (mLightCount >= MaxLights) ReturnFalse(mpLogFile, ERR_NUM_LIGHTS_LIMIT);

	auto newLight = std::make_unique<Light>();
	newLight->Type = type;

	if (type == LightType::E_Directional) {
		newLight->Intensity = 1.f;
		newLight->Color = Vector3(1.f);

		auto dir = Vector3(0.5f, -0.5f, 0.5f);
		dir.Normalize();

		newLight->Direction = dir;
	}
	else if (type == LightType::E_Point) {
		newLight->Intensity = 1.f;
		newLight->Color = Vector3(1.f);

		newLight->Position = Vector3(0.f);
		newLight->Radius = 1.f;
		newLight->AttenuationRadius = 1.f;
	}
	else if (type == LightType::E_Spot) {
		newLight->Intensity = 1.f;
		newLight->Color = Vector3(1.f);

		newLight->Position = Vector3(0.f);
		newLight->OuterConeAngle = 90.f;
		newLight->InnerConeAngle = 1.f;

		newLight->AttenuationRadius = 1.f;
	}
	else if (type == LightType::E_Tube) {
		newLight->Intensity = 1.f;
		newLight->Color = Vector3(1.f);

		newLight->Position = Vector3(-0.5f, 0.f, 0.f);
		newLight->Position1 = Vector3(0.5f, 0.f, 0.f);
		newLight->Radius = 1.f;
		newLight->AttenuationRadius = 1.f;
	}
	else if (type == LightType::E_Rect) {
		newLight->Intensity = 1.f;
		newLight->Color = Vector3(1.f);

		newLight->Center = Vector3(0.f);
		newLight->Up = Vector3(0.f, -1.f, 0.f);
		newLight->Right = Vector3(1.f, 0.f, 0.f);
		newLight->Size = Vector2(1.f);
	}
}