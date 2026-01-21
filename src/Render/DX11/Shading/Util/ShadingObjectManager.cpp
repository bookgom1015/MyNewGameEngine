#include "Render/DX11/Foundation/Core/pch_d3d11.h"
#include "Render/DX11/Shading/Util/ShadingObjectManager.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX11/Shading/Util/ShaderManager.hpp"

using namespace Render::DX11::Shading::Util;

ShadingObjectManager::ShadingObjectManager() {}

ShadingObjectManager::~ShadingObjectManager() {}

BOOL ShadingObjectManager::Initialize(Common::Debug::LogFile* const pLogFile) {
	mpLogFile = pLogFile;

	return TRUE;
}

void ShadingObjectManager::CleanUp() {
	mShadingObjectRefs.clear();
	mShadingObjects.clear();
}

BOOL ShadingObjectManager::CompileShaders(Shading::Util::ShaderManager* const pShaderManager, LPCWSTR baseDir) {
	for (const auto& obj : mShadingObjects)
		CheckReturn(mpLogFile, obj->CompileShaders());

	CheckReturn(mpLogFile, pShaderManager->CompileShaders(baseDir));

	return TRUE;
}

BOOL ShadingObjectManager::BuildPipelineStates() {
	for (const auto& obj : mShadingObjects)
		CheckReturn(mpLogFile, obj->BuildPipelineStates());

	return TRUE;
}

BOOL ShadingObjectManager::BuildDescriptors() {
	for (const auto& obj : mShadingObjects)
		CheckReturn(mpLogFile, obj->BuildDescriptors());

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