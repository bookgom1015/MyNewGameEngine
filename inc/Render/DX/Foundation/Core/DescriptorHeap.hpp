#pragma once

#include <wrl.h>

#include "Render/DX/Foundation/Util/d3dx12.h"

namespace Common::Debug {
	struct LogFile;
}

namespace Render::DX::Foundation::Core {
	class SwapChain;
	class DepthStencilBuffer;

	class DescriptorHeap {
	public:
		DescriptorHeap() = default;
		virtual ~DescriptorHeap() = default;

	public:
		BOOL Initialize(
			Common::Debug::LogFile* const pLogFile, 
			ID3D12Device5* const pDevice, 
			SwapChain* const pSwapChain, 
			DepthStencilBuffer* const pDepthStencilBuffer);
		BOOL CreateDescriptorHeaps(UINT numCbvSrvUav, UINT numRtv, UINT numDsv);
		BOOL BuildDescriptors();
		
		__forceinline D3D12_CPU_DESCRIPTOR_HANDLE CbvSrvUavCpuOffset(UINT offset);
		__forceinline D3D12_GPU_DESCRIPTOR_HANDLE CbvSrvUavGpuOffset(UINT offset);
		__forceinline D3D12_CPU_DESCRIPTOR_HANDLE RtvCpuOffset(UINT offset);
		__forceinline D3D12_CPU_DESCRIPTOR_HANDLE DsvCpuOffset(UINT offset);

	private:
		BOOL BuildDescriptorSizes();

	private:
		Common::Debug::LogFile* mpLogFile = nullptr;

		ID3D12Device5* md3dDevice = nullptr;

		SwapChain* mpSwapChain = nullptr;
		DepthStencilBuffer* mpDepthStencilBuffer = nullptr;

		// Descriptor heaps
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mCbvSrvUavHeap;

		// Descriptor handle sizes
		UINT mRtvDescriptorSize = 0;
		UINT mDsvDescriptorSize = 0;
		UINT mCbvSrvUavDescriptorSize = 0;

		// Descriptor handles
		CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuCbvSrvUav;
		CD3DX12_GPU_DESCRIPTOR_HANDLE mhGpuCbvSrvUav;
		CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuDsv;
		CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuRtv;
	};
}

#include "Render/DX/Foundation/Core/DescriptorHeap.inl"