#include "Render/VK/Shading/Util/ShadingObjectManager.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/VK/Foundation/ShadingObject.hpp"
#include "Render/VK/Shading/Util/ShaderManager.hpp"

using namespace Render::VK::Shading::Util;

BOOL ShadingObjectManager::Initialize(Common::Debug::LogFile* const pLogFile) {
	mpLogFile = pLogFile;

	return TRUE;
}

void ShadingObjectManager::AddShadingObject(Foundation::ShadingObject* const pShadingObject) {
	mShadingObjects.push_back(pShadingObject);
}

BOOL ShadingObjectManager::CompileShaders(Shading::Util::ShaderManager* const pShaderManager, LPCWSTR baseDir) {
	for (const auto object : mShadingObjects)
		CheckReturn(mpLogFile, object->CompileShaders());
	
	CheckReturn(mpLogFile, pShaderManager->CompileShaders(baseDir));

	return TRUE;
}

BOOL ShadingObjectManager::BuildDescriptorSets() {
	for (const auto object : mShadingObjects)
		CheckReturn(mpLogFile, object->BuildDescriptorSets());

	return TRUE;
}

BOOL ShadingObjectManager::BuildPipelineLayouts() {
	for (const auto object : mShadingObjects)
		CheckReturn(mpLogFile, object->BuildPipelineLayouts());

	return TRUE;
}

BOOL ShadingObjectManager::BuildPipelineStates() {
	for (const auto object : mShadingObjects)
		CheckReturn(mpLogFile, object->BuildPipelineStates());

	return TRUE;
}

BOOL ShadingObjectManager::OnResize(UINT width, UINT height) {
	for (const auto object : mShadingObjects)
		CheckReturn(mpLogFile, object->OnResize(width, height));

	return TRUE;
}