#include "Render/DX/Foundation/Core/pch_d3d12.h"
#include "Render/DX/Shading/Util/AccelerationStructure.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX/Foundation/RenderItem.hpp"
#include "Render/DX/Foundation/Resource/GpuResource.hpp"
#include "Render/DX/Foundation/Resource/MeshGeometry.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Foundation/Core/CommandObject.hpp"
#include "Render/DX/Foundation/Resource/FrameResource.hpp"
#include "Render/DX/Foundation/Util/D3D12Util.hpp"

using namespace Render::DX::Shading::Util;

AccelerationStructureBuffer::AccelerationStructureBuffer() {}

AccelerationStructureBuffer::~AccelerationStructureBuffer() { CleanUp(); }

void AccelerationStructureBuffer::CleanUp() {
	if (mInstanceDesc) {
		mInstanceDesc->Unmap(0, nullptr);
		mInstanceDesc.Reset();
		mpMappedData = nullptr;
	}
	if (mResult) mResult.Reset();
	if (mScratch) mScratch.Reset();
}

BOOL AccelerationStructureBuffer::BuildBLAS(
		Common::Debug::LogFile* const pLogFile, 
		Foundation::Core::Device* const pDevice, 
		ID3D12GraphicsCommandList6* const pCmdList, 
		Foundation::Resource::MeshGeometry* const pMeshGeo) {
	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc{};
	geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	geometryDesc.Triangles.VertexCount = pMeshGeo->VertexBufferByteSize / pMeshGeo->VertexByteStride;
	geometryDesc.Triangles.VertexBuffer.StartAddress = pMeshGeo->VertexBufferGPU->GetGPUVirtualAddress();
	geometryDesc.Triangles.VertexBuffer.StrideInBytes = pMeshGeo->VertexByteStride;
	geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
	geometryDesc.Triangles.IndexCount = pMeshGeo->IndexBufferByteSize / pMeshGeo->IndexByteStride;
	geometryDesc.Triangles.IndexBuffer = pMeshGeo->IndexBufferGPU->GetGPUVirtualAddress();
	geometryDesc.Triangles.Transform3x4 = 0;
	// Mark the geometry as opaque. 
	// PERFORMANCE TIP: mark geometry as opaque whenever applicable as it can enable important ray processing optimizations.
	// Note: When rays encounter opaque geometry an any hit shader will not be executed whether it is present or not.
	geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

	// Get the size requirements for the BLAS buffers
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.pGeometryDescs = &geometryDesc;
	inputs.NumDescs = 1;
	inputs.Flags = buildFlags;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo{};
	pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

	prebuildInfo.ScratchDataSizeInBytes = Align(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, prebuildInfo.ScratchDataSizeInBytes);
	prebuildInfo.ResultDataMaxSizeInBytes = Align(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, prebuildInfo.ResultDataMaxSizeInBytes);

	// Create the BLAS scratch buffer
	Foundation::Util::D3D12Util::D3D12BufferCreateInfo bufferInfo(prebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
	bufferInfo.Alignment = std::max(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
	CheckReturn(pLogFile, Foundation::Util::D3D12Util::CreateBuffer(pDevice, bufferInfo, IID_PPV_ARGS(&mScratch)));

	// Create the BLAS buffer
	bufferInfo.Size = prebuildInfo.ResultDataMaxSizeInBytes;
	bufferInfo.State = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
	CheckReturn(pLogFile, Foundation::Util::D3D12Util::CreateBuffer(pDevice, bufferInfo, IID_PPV_ARGS(&mResult)));

	// Describe and build the bottom level acceleration structure
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc{};
	buildDesc.Inputs = inputs;
	buildDesc.ScratchAccelerationStructureData = mScratch->GetGPUVirtualAddress();
	buildDesc.DestAccelerationStructureData = mResult->GetGPUVirtualAddress();

	pCmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);
	Foundation::Util::D3D12Util::UavBarrier(pCmdList, mResult.Get());

	return TRUE;
}

BOOL AccelerationStructureBuffer::BuildTLAS(
		Common::Debug::LogFile* const pLogFile,
		Foundation::Core::Device* const pDevice,
		ID3D12GraphicsCommandList6* const pCmdList,
		D3D12_RAYTRACING_INSTANCE_DESC instanceDescs[],
		UINT numInstanceDescs) {
	const UINT InstanceDescsByteSize = numInstanceDescs * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);

	// Create the TLAS instance buffer
	Foundation::Util::D3D12Util::D3D12BufferCreateInfo instanceBufferInfo;
	instanceBufferInfo.Size = InstanceDescsByteSize;
	instanceBufferInfo.HeapType = D3D12_HEAP_TYPE_UPLOAD;
	instanceBufferInfo.Flags = D3D12_RESOURCE_FLAG_NONE;
	instanceBufferInfo.State = D3D12_RESOURCE_STATE_GENERIC_READ;
	CheckReturn(pLogFile, Foundation::Util::D3D12Util::CreateBuffer(pDevice, instanceBufferInfo, IID_PPV_ARGS(&mInstanceDesc)));

	// Copy the instance data to the buffer
	CheckHRESULT(pLogFile, mInstanceDesc->Map(0, nullptr, &mpMappedData));
	std::memcpy(mpMappedData, instanceDescs, InstanceDescsByteSize);

	// Get the size requirements for the TLAS buffers
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.InstanceDescs = mInstanceDesc->GetGPUVirtualAddress();
	inputs.NumDescs = numInstanceDescs;
	inputs.Flags = buildFlags;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo{};
	pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

	prebuildInfo.ResultDataMaxSizeInBytes = Align(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, prebuildInfo.ResultDataMaxSizeInBytes);
	prebuildInfo.ScratchDataSizeInBytes = Align(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, prebuildInfo.ScratchDataSizeInBytes);

	// Set TLAS size
	mResultDataMaxSizeInBytes = prebuildInfo.ResultDataMaxSizeInBytes;

	// Create TLAS sratch buffer
	Foundation::Util::D3D12Util::D3D12BufferCreateInfo bufferInfo(prebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
	bufferInfo.Alignment = std::max(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
	CheckReturn(pLogFile, Foundation::Util::D3D12Util::CreateBuffer(pDevice, bufferInfo, IID_PPV_ARGS(&mScratch)));

	// Create the TLAS buffer
	bufferInfo.Size = prebuildInfo.ResultDataMaxSizeInBytes;
	bufferInfo.State = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
	CheckReturn(pLogFile, Foundation::Util::D3D12Util::CreateBuffer(pDevice, bufferInfo, IID_PPV_ARGS(&mResult)));

	// Describe and build the TLAS
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc{};
	buildDesc.Inputs = inputs;
	buildDesc.ScratchAccelerationStructureData = mScratch->GetGPUVirtualAddress();
	buildDesc.DestAccelerationStructureData = mResult->GetGPUVirtualAddress();
	pCmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

	// Wait for the TLAS build to complete
	Foundation::Util::D3D12Util::UavBarrier(pCmdList, mResult.Get());

	return TRUE;
}

BOOL AccelerationStructureBuffer::UpdateTLAS(
		Common::Debug::LogFile* const pLogFile,
		ID3D12GraphicsCommandList6* const pCmdList,
		D3D12_RAYTRACING_INSTANCE_DESC instanceDescs[],
		UINT numInstanceDescs) {
	std::memcpy(mpMappedData, instanceDescs, numInstanceDescs * sizeof(D3D12_RAYTRACING_INSTANCE_DESC));

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.InstanceDescs = mInstanceDesc->GetGPUVirtualAddress();
	inputs.NumDescs = numInstanceDescs;
	inputs.Flags = buildFlags;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc{};
	buildDesc.Inputs = inputs;
	buildDesc.ScratchAccelerationStructureData = mScratch->GetGPUVirtualAddress();
	buildDesc.DestAccelerationStructureData = mResult->GetGPUVirtualAddress();
	pCmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

	return TRUE;
}

AccelerationStructureManager::AccelerationStructureManager() {
	mTLAS = std::make_unique<AccelerationStructureBuffer>();
}

AccelerationStructureManager::~AccelerationStructureManager() { CleanUp(); }

BOOL AccelerationStructureManager::Initialize(
		Common::Debug::LogFile* const pLogFile,
		Foundation::Core::Device* const pDevice,
		Foundation::Core::CommandObject* const pCmdObject) {
	mpLogFile = pLogFile;
	mpDevice = pDevice;
	mpCommandObject = pCmdObject;

	return TRUE;
}

void AccelerationStructureManager::CleanUp() {
	if (mTLAS) mTLAS.reset();

	mBLASRefs.clear();
	
	for (auto& blas : mBLASes)
		blas.reset();
	mBLASes.clear();
	
	mpCommandObject = nullptr;
	mpDevice = nullptr;
	mpLogFile = nullptr;
}

BOOL AccelerationStructureManager::BuildBLAS(
		ID3D12GraphicsCommandList6* const pCmdList,
		Foundation::Resource::MeshGeometry* const  pMeshGeo) {
	const auto Hash = Foundation::Resource::MeshGeometry::Hash(pMeshGeo);
	if (mBLASRefs.find(Hash) != mBLASRefs.end()) return TRUE;

	std::unique_ptr<AccelerationStructureBuffer> blas = std::make_unique<AccelerationStructureBuffer>();

	CheckReturn(mpLogFile, blas->BuildBLAS(mpLogFile, mpDevice, pCmdList, pMeshGeo));

	mBLASRefs[Hash] = blas.get();
	mBLASes.emplace_back(std::move(blas));

	mbNeedToRebuildTLAS = TRUE;

	return TRUE;
}

BOOL AccelerationStructureManager::Update(
		Foundation::Resource::FrameResource* const pFrameResource,
		Foundation::RenderItem* const ritems[],
		UINT numRitems) {
	CheckReturn(mpLogFile, mpCommandObject->ResetCommandList(
		pFrameResource->CommandAllocator(0),
		0));

	const auto CmdList = mpCommandObject->CommandList(0);

	if (mbNeedToRebuildTLAS) {
		CheckReturn(mpLogFile, BuildTLAS(CmdList, ritems, numRitems));

		mbNeedToRebuildTLAS = FALSE;
	}
	else {
		CheckReturn(mpLogFile, UpdateTLAS(CmdList, ritems, numRitems));
	}

	CheckReturn(mpLogFile, mpCommandObject->ExecuteCommandList(0));

	return TRUE;
}

BOOL AccelerationStructureManager::BuildTLAS(
		ID3D12GraphicsCommandList6* const pCmdList,
		Foundation::RenderItem* const ritems[],
		UINT numRitems) {
	std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;
	CheckReturn(mpLogFile, BuildInstanceDescriptors(instanceDescs, ritems, numRitems));

	if (instanceDescs.size() == 0) return TRUE;

	CheckReturn(mpLogFile, mTLAS->BuildTLAS(
		mpLogFile,
		mpDevice,
		pCmdList,
		instanceDescs.data(),
		static_cast<UINT>(instanceDescs.size())));

	return TRUE;
}

BOOL AccelerationStructureManager::UpdateTLAS(
		ID3D12GraphicsCommandList6* const pCmdList,
		Foundation::RenderItem* const ritems[],
		UINT numRitems) {
	std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;
	CheckReturn(mpLogFile, BuildInstanceDescriptors(instanceDescs, ritems, numRitems));

	if (instanceDescs.size() == 0) return TRUE;

	CheckReturn(mpLogFile, mTLAS->UpdateTLAS(
		mpLogFile,
		pCmdList,
		instanceDescs.data(),
		static_cast<UINT>(instanceDescs.size())));

	return TRUE;
}

BOOL AccelerationStructureManager::BuildInstanceDescriptors(
		std::vector<D3D12_RAYTRACING_INSTANCE_DESC>& instanceDescs,
		Foundation::RenderItem* const ritems[],
		UINT numRitems) {
	for (UINT i = 0; i < numRitems; ++i) {
		const auto ri = ritems[i];

		if (!ri->RebuildAccerationStructure) continue;

		const UINT HitGroupIndex = i;
		const auto Hash = Foundation::Resource::MeshGeometry::Hash(ri->Geometry);
		if (mBLASRefs.find(Hash) == mBLASRefs.end()) ReturnFalse(mpLogFile, L"Failed to find BLAS");

		D3D12_RAYTRACING_INSTANCE_DESC instanceDesc{};
		instanceDesc.InstanceID = 0;
		instanceDesc.InstanceContributionToHitGroupIndex = HitGroupIndex;
		instanceDesc.InstanceMask = 0xFF;
		for (INT r = 0; r < 3; ++r) {
			for (INT c = 0; c < 4; ++c) {
				instanceDesc.Transform[r][c] = ri->World.m[c][r];
			}
		}
		instanceDesc.AccelerationStructure = mBLASRefs[Hash]->mResult->GetGPUVirtualAddress();
		instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
		instanceDescs.push_back(instanceDesc);
	}

	return TRUE;
}