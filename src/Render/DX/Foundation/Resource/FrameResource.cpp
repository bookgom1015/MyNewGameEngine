#include "Render/DX/Foundation/Resource/FrameResource.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"

using namespace Render::DX::Foundation::Resource;

UINT FrameResource::MainPassCBByteSize() const {
	return Util::D3D12Util::CalcConstantBufferByteSize(sizeof(ConstantBuffers::PassCB));
}

UINT FrameResource::ObjectCBByteSize() const {
	return Util::D3D12Util::CalcConstantBufferByteSize(sizeof(ConstantBuffers::ObjectCB));
}

UINT FrameResource::EquirectConvCBByteSize() const {
	return Util::D3D12Util::CalcConstantBufferByteSize(sizeof(ConstantBuffers::EquirectangularConverterCB));
}

BOOL FrameResource::Initialize(
		Common::Debug::LogFile* const pLogFile, 
		Core::Device* const pDevice, 
		UINT numThreads,
		UINT numPasses, 
		UINT numObjects) {
	mpLogFile = pLogFile;
	mpDevice = pDevice;

	mThreadCount = numThreads;

	mCmdAllocators.resize(mThreadCount);

	CheckReturn(mpLogFile, CreateCommandListAllocators());
	CheckReturn(mpLogFile, BuildConstantBuffres(numPasses, numObjects));

	return TRUE;
}

BOOL FrameResource::CreateCommandListAllocators() {
	for (UINT i = 0, end = static_cast<UINT>(mCmdAllocators.size()); i < end; ++i) 
		CheckReturn(mpLogFile, mpDevice->CreateCommandAllocator(mCmdAllocators[i]));

	return TRUE;
}

BOOL FrameResource::BuildConstantBuffres(
		UINT numPasses,
		UINT numObjects) {
	CheckReturn(mpLogFile, mMainPassCB.Initialize(mpLogFile, mpDevice, numPasses, TRUE));
	CheckReturn(mpLogFile, mObjectCB.Initialize(mpLogFile, mpDevice, numObjects, TRUE));
	CheckReturn(mpLogFile, mEquirectConvCB.Initialize(mpLogFile, mpDevice, 1, TRUE));

	return TRUE;
}