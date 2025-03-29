#pragma once

#include "wrl.h"

#include "Render/DX/Foundation/Util/d3dx12.h"

namespace Common::Debug {
	struct LogFile;
}

namespace Render::DX::Foundation {	
	namespace Core {
		class Device;
	}

	namespace Util {
		class GpuResource;

		class D3D12Util {
		public:
			static BOOL CreateDefaultBuffer(
				Common::Debug::LogFile* const pLogFile,
				Core::Device* const pDevice,
				ID3D12GraphicsCommandList4* const cmdList,
				const void* const pInitData,
				UINT64 byteSize,
				Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer,
				Microsoft::WRL::ComPtr<ID3D12Resource>& defaultBuffer);

			static void UavBarrier(ID3D12GraphicsCommandList* const pCmdList, ID3D12Resource* pResource);
			static void UavBarriers(ID3D12GraphicsCommandList* const pCmdList, ID3D12Resource* pResources[], UINT length);
			static void UavBarrier(ID3D12GraphicsCommandList* const pCmdList, GpuResource* pResource);
			static void UavBarriers(ID3D12GraphicsCommandList* const pCmdList, GpuResource* pResources[], UINT length);

			static void CreateShaderResourceView(
				Core::Device* const pDevice,
				ID3D12Resource* const pResource,
				const D3D12_SHADER_RESOURCE_VIEW_DESC* const pDesc,
				D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor);
			static void CreateRenderTargetView(
				Core::Device* const pDevice,
				ID3D12Resource* const pResource,
				const D3D12_RENDER_TARGET_VIEW_DESC* const pDesc,
				D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor);
			static void CreateDepthStencilView(
				Core::Device* const pDevice,
				ID3D12Resource* const pResource,
				const D3D12_DEPTH_STENCIL_VIEW_DESC* const pDesc,
				D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor);
		};
	}
}