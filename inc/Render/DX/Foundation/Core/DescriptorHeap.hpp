#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <wrl.h>
#include <Windows.h>

#include <Microsoft.Direct3D.D3D12.1.615.1/build/native/include/d3dx12/d3dx12.h>

namespace Common::Debug {
	struct LogFile;
}

namespace Render::DX::Foundation::Core {
	class Device;
	class SwapChain;
	class DepthStencilBuffer;

	class DescriptorHeap {
	public:
		DescriptorHeap() = default;
		virtual ~DescriptorHeap() = default;

	public:
		__forceinline D3D12_CPU_DESCRIPTOR_HANDLE CbvSrvUavCpuOffset(UINT offset);
		__forceinline D3D12_GPU_DESCRIPTOR_HANDLE CbvSrvUavGpuOffset(UINT offset);
		__forceinline D3D12_CPU_DESCRIPTOR_HANDLE RtvCpuOffset(UINT offset);
		__forceinline D3D12_CPU_DESCRIPTOR_HANDLE DsvCpuOffset(UINT offset);

	public:
		BOOL Initialize(
			Common::Debug::LogFile* const pLogFile, 
			Device* const pDevice,
			SwapChain* const pSwapChain, 
			DepthStencilBuffer* const pDepthStencilBuffer);

		BOOL CreateDescriptorHeaps(UINT numCbvSrvUav, UINT numRtv, UINT numDsv);
		BOOL BuildDescriptors();

		BOOL SetDescriptorHeap(ID3D12GraphicsCommandList4* const pCmdList);

	private:
		BOOL BuildDescriptorSizes();

	private:
		Common::Debug::LogFile* mpLogFile = nullptr;

		Device* mDevice = nullptr;

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