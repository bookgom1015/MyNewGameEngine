#include "Render/DX/Foundation/Resource/FrameResource.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"

using namespace Render::DX::Foundation::Resource;

UINT FrameResource::MainPassCBByteSize() const {
	return Util::D3D12Util::CalcConstantBufferByteSize(sizeof(ConstantBuffers::PassCB));
}

UINT FrameResource::LightCBByteSize() const {
	return Util::D3D12Util::CalcConstantBufferByteSize(sizeof(ConstantBuffers::LightCB));
}

UINT FrameResource::ObjectCBByteSize() const {
	return Util::D3D12Util::CalcConstantBufferByteSize(sizeof(ConstantBuffers::ObjectCB));
}

UINT FrameResource::MaterialCBByteSize() const {
	return Util::D3D12Util::CalcConstantBufferByteSize(sizeof(ConstantBuffers::MaterialCB));
}

UINT FrameResource::ProjectToCubeCBByteSize() const {
	return Util::D3D12Util::CalcConstantBufferByteSize(sizeof(ConstantBuffers::ProjectToCubeCB));
}

UINT FrameResource::AmbientOcclusionCBByteSize() const {
	return Util::D3D12Util::CalcConstantBufferByteSize(sizeof(ConstantBuffers::AmbientOcclusionCB));
}

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
	CheckReturn(mpLogFile, mMainPassCB.Initialize(mpLogFile, mpDevice, numPasses, TRUE));
	CheckReturn(mpLogFile, mLightCB.Initialize(mpLogFile, mpDevice, 1, TRUE));
	CheckReturn(mpLogFile, mObjectCB.Initialize(mpLogFile, mpDevice, numObjects, TRUE));
	CheckReturn(mpLogFile, mMaterialCB.Initialize(mpLogFile, mpDevice, numMaterials, TRUE));
	CheckReturn(mpLogFile, mProjectToCubeCB.Initialize(mpLogFile, mpDevice, 1, TRUE));
	CheckReturn(mpLogFile, mAmbientOcclusionCB.Initialize(mpLogFile, mpDevice, 1, TRUE));

	return TRUE;
}