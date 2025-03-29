#include "Render/DX/Foundation/Util/D3D12Util.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Foundation/Util/GpuResource.hpp"

#include <vector>

using namespace Render::DX::Foundation::Util;
using namespace Microsoft::WRL;

BOOL D3D12Util::CreateDefaultBuffer(
		Common::Debug::LogFile* const pLogFile,
		Core::Device* const pDevice,
		ID3D12GraphicsCommandList4* const cmdList,
		const void* const pInitData,
		UINT64 byteSize,
		ComPtr<ID3D12Resource>& uploadBuffer,
		ComPtr<ID3D12Resource>& defaultBuffer) {
	// Create the actual default buffer resource.
	CheckHRESULT(pLogFile, pDevice->md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(defaultBuffer.GetAddressOf()))
	);

	// In order to copy CPU memory data into our default buffer, we need to create
	// an intermediate upload heap. 
	CheckHRESULT(pLogFile, pDevice->md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(uploadBuffer.GetAddressOf()))
	);

	// Describe the data we want to copy into the default buffer.
	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = pInitData;
	subResourceData.RowPitch = byteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;

	// Schedule to copy the data to the default buffer resource.  At a high level, the helper function UpdateSubresources
	// will copy the CPU memory into the intermediate upload heap.  Then, using ID3D12CommandList::CopySubresourceRegion,
	// the intermediate upload heap data will be copied to mBuffer.
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));

	UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

	// Note: uploadBuffer has to be kept alive after the above function calls because
	// the command list has not been executed yet that performs the actual copy.
	// The caller can Release the uploadBuffer after it knows the copy has been executed.

	return TRUE;
}

void D3D12Util::UavBarrier(ID3D12GraphicsCommandList* const pCmdList, ID3D12Resource* pResource) {
	D3D12_RESOURCE_BARRIER uavBarrier;
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = pResource;
	uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	pCmdList->ResourceBarrier(1, &uavBarrier);
}

void D3D12Util::UavBarriers(ID3D12GraphicsCommandList* const pCmdList, ID3D12Resource* pResources[], UINT length) {
	std::vector<D3D12_RESOURCE_BARRIER> uavBarriers;
	for (UINT i = 0; i < length; ++i) {
		D3D12_RESOURCE_BARRIER uavBarrier;
		uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		uavBarrier.UAV.pResource = pResources[i];
		uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		uavBarriers.push_back(uavBarrier);
	}
	pCmdList->ResourceBarrier(static_cast<UINT>(uavBarriers.size()), uavBarriers.data());
}

void D3D12Util::UavBarrier(ID3D12GraphicsCommandList* const pCmdList, GpuResource* pResource) {
	D3D12_RESOURCE_BARRIER uavBarrier;
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = pResource->Resource();
	uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	pCmdList->ResourceBarrier(1, &uavBarrier);
}

void D3D12Util::UavBarriers(ID3D12GraphicsCommandList* const pCmdList, GpuResource* pResources[], UINT length) {
	std::vector<D3D12_RESOURCE_BARRIER> uavBarriers;
	for (UINT i = 0; i < length; ++i) {
		D3D12_RESOURCE_BARRIER uavBarrier;
		uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		uavBarrier.UAV.pResource = pResources[i]->Resource();
		uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		uavBarriers.push_back(uavBarrier);
	}
	pCmdList->ResourceBarrier(static_cast<UINT>(uavBarriers.size()), uavBarriers.data());
}

void D3D12Util::CreateShaderResourceView(
		Core::Device* const pDevice,
		ID3D12Resource* const pResource,
		const D3D12_SHADER_RESOURCE_VIEW_DESC* const pDesc,
		D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor) {
	pDevice->md3dDevice->CreateShaderResourceView(pResource, pDesc, destDescriptor);
}

void D3D12Util::CreateRenderTargetView(
		Core::Device* const pDevice,
		ID3D12Resource* const pResource,
		const D3D12_RENDER_TARGET_VIEW_DESC* const pDesc,
		D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor) {
	pDevice->md3dDevice->CreateRenderTargetView(pResource, pDesc, destDescriptor);
}

void D3D12Util::CreateDepthStencilView(
		Core::Device* const pDevice,
		ID3D12Resource* const pResource,
		const D3D12_DEPTH_STENCIL_VIEW_DESC* const pDesc,
		D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor) {
	pDevice->md3dDevice->CreateDepthStencilView(pResource, pDesc, destDescriptor);
}