#pragma once

namespace Common::Debug {
	struct LogFile;
}

namespace ImGuiManager::DX {
	class DxImGuiManager;
}

namespace Render::DX {
	namespace Foundation::Core {
		class Device;
		class SwapChain;
		class DepthStencilBuffer;

		class DescriptorHeap {
		private:
			friend class ImGuiManager::DX::DxImGuiManager;

		public:
			DescriptorHeap();
			virtual ~DescriptorHeap();

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
			void CleanUp();

			BOOL CreateDescriptorHeaps(UINT numCbvSrvUav, UINT numRtv, UINT numDsv);
			BOOL BuildDescriptors();

			BOOL SetDescriptorHeap(ID3D12GraphicsCommandList4* const pCmdList);

		private:
			BOOL BuildDescriptorSizes();

		private:
			BOOL mbCleanedUp{};
			Common::Debug::LogFile* mpLogFile{};

			Device* mDevice{};

			SwapChain* mpSwapChain{};
			DepthStencilBuffer* mpDepthStencilBuffer{};

			// Descriptor heaps
			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap{};
			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap{};
			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mCbvSrvUavHeap{};

			// Descriptor handle sizes
			UINT mRtvDescriptorSize{};
			UINT mDsvDescriptorSize{};
			UINT mCbvSrvUavDescriptorSize{};

			// Descriptor handles
			CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuCbvSrvUav{};
			CD3DX12_GPU_DESCRIPTOR_HANDLE mhGpuCbvSrvUav{};
			CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuDsv{};
			CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuRtv{};
		};
	}
}

#include "Render/DX/Foundation/Core/DescriptorHeap.inl"