#include "Render/DX/Foundation/Core/pch_d3d12.h"
#include "Render/DX/Foundation/Resource/FrameResource.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"

using namespace Render::DX::Foundation::Resource;

FrameResource::FrameResource() {}

FrameResource::~FrameResource() { CleanUp(); }

BOOL FrameResource::Initialize(
		Common::Debug::LogFile* const pLogFile, 
		Core::Device* const pDevice, 
		UINT numThreads,
		UINT numPasses, 
		UINT numObjects,
		UINT numMaterials) {
	mpLogFile = pLogFile;
	mpDevice = pDevice;

	mThreadCount = numThreads;

	mCmdAllocators.resize(mThreadCount);

	CheckReturn(mpLogFile, CreateCommandListAllocators());
	CheckReturn(mpLogFile, BuildConstantBuffres(numPasses, numObjects, numMaterials));

	return TRUE;
}

void FrameResource::CleanUp() {
	if (mbCleanedUp) return;

	for (UINT i = 0; i < Count; ++i) {
		auto& allocator = mCmdAllocators[i];
		if (allocator) allocator.Reset();
	}
	mCmdAllocators.clear();

	mpDevice = nullptr;
	mpLogFile = nullptr;

	mbCleanedUp = TRUE;
}

BOOL FrameResource::ResetCommandListAllocators() {
	for (UINT i = 0; i < mThreadCount; ++i)
		CheckHRESULT(mpLogFile, mCmdAllocators[i]->Reset());

	return TRUE;
}

BOOL FrameResource::CreateCommandListAllocators() {
	for (UINT i = 0, end = static_cast<UINT>(mCmdAllocators.size()); i < end; ++i) 
		CheckReturn(mpLogFile, mpDevice->CreateCommandAllocator(mCmdAllocators[i]));

	return TRUE;
}

BOOL FrameResource::BuildConstantBuffres(
		UINT numPasses,
		UINT numObjects,
		UINT numMaterials) {
	CheckReturn(mpLogFile, MainPassCB.Initialize(mpLogFile, mpDevice, numPasses, 1, TRUE));
	CheckReturn(mpLogFile, LightCB.Initialize(mpLogFile, mpDevice, 1, 1, TRUE));
	CheckReturn(mpLogFile, ObjectCB.Initialize(mpLogFile, mpDevice, numObjects, 1, TRUE));
	CheckReturn(mpLogFile, MaterialCB.Initialize(mpLogFile, mpDevice, numMaterials, 1, TRUE));
	CheckReturn(mpLogFile, ProjectToCubeCB.Initialize(mpLogFile, mpDevice, 1, 1, TRUE));
	CheckReturn(mpLogFile, AmbientOcclusionCB.Initialize(mpLogFile, mpDevice, 1, 1, TRUE));
	CheckReturn(mpLogFile, RayGenCB.Initialize(mpLogFile, mpDevice, 1, 1, TRUE));
	CheckReturn(mpLogFile, RaySortingCB.Initialize(mpLogFile, mpDevice, 1, 1, TRUE));
	CheckReturn(mpLogFile, CrossBilateralFilterCB.Initialize(mpLogFile, mpDevice, 1, 1, TRUE));
	CheckReturn(mpLogFile, CalcLocalMeanVarianceCB.Initialize(mpLogFile, mpDevice, 1, 1, TRUE));
	CheckReturn(mpLogFile, BlendWithCurrentFrameCB.Initialize(mpLogFile, mpDevice, 1, 1, TRUE));
	CheckReturn(mpLogFile, AtrousWaveletTransformFilterCB.Initialize(mpLogFile, mpDevice, 1, 1, TRUE));
	CheckReturn(mpLogFile, ContactShadowCB.Initialize(mpLogFile, mpDevice, 1, 1, TRUE));

	return TRUE;
}