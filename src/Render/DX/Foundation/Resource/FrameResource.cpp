#include "Render/DX/Foundation/Resource/FrameResource.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"

using namespace Render::DX::Foundation::Resource;

BOOL FrameResource::Initialize(
		Common::Debug::LogFile* const pLogFile, 
		Core::Device* const pDevice, 
		UINT numThreads,
		UINT numPasses, 
		UINT numObjects) {
	mpLogFile = pLogFile;
	mpDevice = pDevice;
	mPassCount = numPasses;
	mObjectCount = numObjects;

	mCmdAllocators.resize(numThreads);

	CheckReturn(mpLogFile, CreateCommandListAllocators());
	CheckReturn(mpLogFile, BuildConstantBuffres());

	return TRUE;
}

BOOL FrameResource::CreateCommandListAllocators() {
	for (UINT i = 0, end = static_cast<UINT>(mCmdAllocators.size()); i < end; ++i) 
		CheckReturn(mpLogFile, mpDevice->CreateCommandAllocator(mCmdAllocators[i]));

	return TRUE;
}

BOOL FrameResource::BuildConstantBuffres() {
	CheckReturn(mpLogFile, mPassCB.Initialize(mpLogFile, mpDevice, mPassCount, TRUE));
	CheckReturn(mpLogFile, mObjectCB.Initialize(mpLogFile, mpDevice, mObjectCount, TRUE));

	return TRUE;
}