#include "Render/DX11/Foundation/Core/pch_d3d11.h"
#include "Render/DX11/Shading/Util/ShadingObjectManager.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX11/Shading/Util/ShaderManager.hpp"

using namespace Render::DX11::Shading::Util;

ShadingObjectManager::ShadingObjectManager() {}

ShadingObjectManager::~ShadingObjectManager() { CleanUp(); }

BOOL ShadingObjectManager::Initialize(Common::Debug::LogFile* const pLogFile) {
	mpLogFile = pLogFile;

	return TRUE;
}

void ShadingObjectManager::CleanUp() {
	if (mbCleanedUp) return;

	for (const auto& obj : mShadingObjects)
		obj->CleanUp();

	mShadingObjectRefs.clear();
	mShadingObjects.clear();

	mbCleanedUp = TRUE;
}

BOOL ShadingObjectManager::CompileShaders() {
	for (const auto& obj : mShadingObjects)
		CheckReturn(mpLogFile, obj->CompileShaders());

	return TRUE;
}

BOOL ShadingObjectManager::BuildPipelineStates() {
	for (const auto& obj : mShadingObjects)
		CheckReturn(mpLogFile, obj->BuildPipelineStates());

	return TRUE;
}

BOOL ShadingObjectManager::OnResize(UINT width, UINT height) {
	for (const auto& obj : mShadingObjects)
		CheckReturn(mpLogFile, obj->OnResize(width, height));

	return TRUE;
}

BOOL ShadingObjectManager::Update() {
	for (const auto& obj : mShadingObjects)
		CheckReturn(mpLogFile, obj->Update());

	return TRUE;
}