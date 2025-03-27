#pragma once

#include "wrl.h"

#include "Render/DX/Foundation/Util/d3dx12.h"

namespace Common::Debug {
	struct LogFile;
}

namespace Render::DX::Foundation::Util {
	class GpuResource;

	class D3D12Util {
	public:
		static BOOL CreateDefaultBuffer(
			Common::Debug::LogFile* const pLogFile,
			ID3D12Device5* const device,
			ID3D12GraphicsCommandList4* const cmdList,
			const void* const initData,
			UINT64 byteSize,
			Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer,
			Microsoft::WRL::ComPtr<ID3D12Resource>& defaultBuffer);

		static void UavBarrier(ID3D12GraphicsCommandList* const pCmdList, ID3D12Resource* pResource);
		static void UavBarriers(ID3D12GraphicsCommandList* const pCmdList, ID3D12Resource* pResources[], UINT length);
		static void UavBarrier(ID3D12GraphicsCommandList* const pCmdList, GpuResource* pResource);
		static void UavBarriers(ID3D12GraphicsCommandList* const pCmdList, GpuResource* pResources[], UINT length);
	};
}