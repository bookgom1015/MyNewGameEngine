#include "Render/VK/Shading/Util/ShadingObjectManager.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/VK/Foundation/ShadingObject.hpp"
#include "Render/VK/Shading/Util/ShaderManager.hpp"

using namespace Render::VK::Shading::Util;

BOOL ShadingObjectManager::Initialize(Common::Debug::LogFile* const pLogFile) {
	mpLogFile = pLogFile;

	return TRUE;
}

void ShadingObjectManager::CleanUp() {
	for (const auto object : mShadingObjects)
		object->CleanUp();
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

BOOL ShadingObjectManager::BuildImages() {
	for (const auto object : mShadingObjects)
		CheckReturn(mpLogFile, object->BuildImages());

	return TRUE;
}

BOOL ShadingObjectManager::BuildImageViews() {
	for (const auto object : mShadingObjects)
		CheckReturn(mpLogFile, object->BuildImageViews());

	return TRUE;
}

BOOL ShadingObjectManager::BuildFixedImages() {
	for (const auto object : mShadingObjects)
		CheckReturn(mpLogFile, object->BuildFixedImages());

	return TRUE;
}

BOOL ShadingObjectManager::BuildFixedImageViews() {
	for (const auto object : mShadingObjects)
		CheckReturn(mpLogFile, object->BuildFixedImageViews());

	return TRUE;
}

BOOL ShadingObjectManager::BuildRenderPass() {
	for (const auto object : mShadingObjects)
		CheckReturn(mpLogFile, object->BuildRenderPass());

	return TRUE;
}

BOOL ShadingObjectManager::BuildPipelineStates() {
	for (const auto object : mShadingObjects)
		CheckReturn(mpLogFile, object->BuildPipelineStates());

	return TRUE;
}

BOOL ShadingObjectManager::BuildFramebuffers() {
	for (const auto object : mShadingObjects)
		CheckReturn(mpLogFile, object->BuildFramebuffers());

	return TRUE;
}

BOOL ShadingObjectManager::OnResize(UINT width, UINT height) {
	for (const auto object : mShadingObjects)
		CheckReturn(mpLogFile, object->OnResize(width, height));

	return TRUE;
}