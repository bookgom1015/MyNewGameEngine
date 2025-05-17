#include "Render/DX/Shading/Util/ShadingObjectManager.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX/Foundation/ShadingObject.hpp"
#include "Render/DX/Shading/Util/ShaderManager.hpp"
#include "Render/DX/Shading/Util/SamplerUtil.hpp"

using namespace Render::DX::Shading::Util;

BOOL ShadingObjectManager::Initialize(Common::Debug::LogFile* const pLogFile) {
	mpLogFile = pLogFile;

	return TRUE;
}

void ShadingObjectManager::AddShadingObject(Foundation::ShadingObject* const pShadingObject) {
	mShadingObjects.push_back(pShadingObject);
}

UINT ShadingObjectManager::CbvSrvUavDescCount() const {
	UINT count = 0;
	for (const auto object : mShadingObjects)
		count += object->CbvSrvUavDescCount();

	return count;
}

UINT ShadingObjectManager::RtvDescCount() const {
	UINT count = 0;
	for (const auto object : mShadingObjects)
		count += object->RtvDescCount();

	return count;
}

UINT ShadingObjectManager::DsvDescCount() const {
	UINT count = 0;
	for (const auto object : mShadingObjects)
		count += object->DsvDescCount();

	return count;
}

BOOL ShadingObjectManager::CompileShaders(Shading::Util::ShaderManager* const pShaderManager, LPCWSTR baseDir) {
	for (const auto object : mShadingObjects)
		CheckReturn(mpLogFile, object->CompileShaders());

	CheckReturn(mpLogFile, pShaderManager->CompileShaders(baseDir));

	return TRUE;
}

BOOL ShadingObjectManager::BuildRootSignatures() {
	const auto& staticSamplers = Shading::Util::SamplerUtil::GetStaticSamplers();

	for (const auto object : mShadingObjects)
		CheckReturn(mpLogFile, object->BuildRootSignatures(staticSamplers));

	return TRUE;
}

BOOL ShadingObjectManager::BuildPipelineStates() {
	for (const auto object : mShadingObjects)
		CheckReturn(mpLogFile, object->BuildPipelineStates());

	return TRUE;
}

BOOL ShadingObjectManager::BuildDescriptors(Foundation::Core::DescriptorHeap* const pDescHeap) {
	for (const auto object : mShadingObjects)
		CheckReturn(mpLogFile, object->BuildDescriptors(pDescHeap));

	return TRUE;
}

BOOL ShadingObjectManager::OnResize(UINT width, UINT height) {
	for (const auto object : mShadingObjects)
		CheckReturn(mpLogFile, object->OnResize(width, height));

	return TRUE;
}

BOOL ShadingObjectManager::BuildShaderTables(UINT numRitems) {
	for (const auto object : mShadingObjects)
		CheckReturn(mpLogFile, object->BuildShaderTables(numRitems));

	return TRUE;
}