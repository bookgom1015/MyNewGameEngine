#pragma once

#include <wrl.h>

#include <dxgi1_6.h>
#include <d3d12.h>

namespace Common::Debug {
	struct LogFile;
}

namespace Render::DX::Foundation {
	namespace Core {
		class Device;
	}

	namespace Resource {
		class GpuResource {
		public:
			GpuResource() = default;
			virtual ~GpuResource() = default;

		public:
			static BOOL Initialize(Common::Debug::LogFile* const pLogFile);

		public:
			BOOL Initialize(
				Core::Device* const pDevice,
				const D3D12_HEAP_PROPERTIES* const pHeapProp,
				D3D12_HEAP_FLAGS heapFlag,
				const D3D12_RESOURCE_DESC* const pRscDesc,
				D3D12_RESOURCE_STATES initialState,
				const D3D12_CLEAR_VALUE* const pOptClear,
				LPCWSTR pName = nullptr);

			BOOL OnResize(IDXGISwapChain* const pSwapChain, UINT index);

			void Swap(Microsoft::WRL::ComPtr<ID3D12Resource>& srcResource);
			void Swap(ID3D12GraphicsCommandList* const pCmdList, Microsoft::WRL::ComPtr<ID3D12Resource>& srcResource, D3D12_RESOURCE_STATES initialState);
			void Transite(ID3D12GraphicsCommandList* const pCmdList, D3D12_RESOURCE_STATES state);

			__forceinline void Reset();

		public:
			__forceinline ID3D12Resource* const Resource() const;
			__forceinline D3D12_RESOURCE_DESC Desc() const;
			__forceinline D3D12_RESOURCE_STATES State() const;

		private:
			static Common::Debug::LogFile* mpLogFile;

		private:
			Microsoft::WRL::ComPtr<ID3D12Resource> mResource;

			D3D12_RESOURCE_STATES mCurrState = D3D12_RESOURCE_STATE_COMMON;
		};
	}
}

#include "GpuResource.inl"