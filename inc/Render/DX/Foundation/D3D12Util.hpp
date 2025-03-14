#pragma once

#include "wrl.h"

#include "Render/DX/Foundation/d3dx12.h"

namespace Render::DX::Foundation {
	class GpuResource;

	class D3D12Util {
	public:
		static void UavBarrier(ID3D12GraphicsCommandList* const pCmdList, ID3D12Resource* pResource);
		static void UavBarriers(ID3D12GraphicsCommandList* const pCmdList, ID3D12Resource* pResources[], UINT length);
		static void UavBarrier(ID3D12GraphicsCommandList* const pCmdList, GpuResource* pResource);
		static void UavBarriers(ID3D12GraphicsCommandList* const pCmdList, GpuResource* pResources[], UINT length);
	};
}