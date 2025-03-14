#include "Render/DX/Foundation/D3D12Util.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX/Foundation/GpuResource.hpp"

#include <vector>

using namespace Render::DX::Foundation;

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
