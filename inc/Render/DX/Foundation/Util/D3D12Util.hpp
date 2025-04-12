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
#include <dxgi.h>

namespace Common::Debug {
	struct LogFile;
}

namespace Render::DX::Foundation {
	namespace Core {
		class Factory;
		class Device;
		class CommandObject;
	}

	namespace Resource {
		class GpuResource;
		struct Texture;
	}

	namespace Util {
		class D3D12Util {
		public:
			static BOOL Initialize(Common::Debug::LogFile* const pLogFile);

		public:
			__forceinline static UINT CeilDivide(UINT value, UINT divisor);

		public:
			static BOOL CreateSwapChain(
				Core::Factory* const pFactory,
				Core::CommandObject* const pCmdObject,
				DXGI_SWAP_CHAIN_DESC* pDesc,
				IDXGISwapChain** ppSwapChain);

			static BOOL CalcConstantBufferByteSize(UINT byteSize);
			static BOOL CreateDefaultBuffer(
				Core::Device* const pDevice,
				ID3D12GraphicsCommandList4* const cmdList,
				const void* const pInitData,
				UINT64 byteSize,
				Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer,
				Microsoft::WRL::ComPtr<ID3D12Resource>& defaultBuffer);
			static BOOL CreateUploadBuffer(
				Core::Device* const pDevice,
				UINT64 byteSize,
				Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);

			static void Transite(
				Core::CommandObject* const pCmdObect, 
				ID3D12Resource* const pResource, 
				D3D12_RESOURCE_STATES stateBefore,
				D3D12_RESOURCE_STATES stateAfter);

			static void UavBarrier(ID3D12GraphicsCommandList* const pCmdList, ID3D12Resource* pResource);
			static void UavBarriers(ID3D12GraphicsCommandList* const pCmdList, ID3D12Resource* pResources[], UINT length);
			static void UavBarrier(ID3D12GraphicsCommandList* const pCmdList, Resource::GpuResource* pResource);
			static void UavBarriers(ID3D12GraphicsCommandList* const pCmdList, Resource::GpuResource* pResources[], UINT length);

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

			static BOOL CreateTexture(
				Core::Device* const pDevice, 
				Core::CommandObject* const pCmdObject, 
				Resource::Texture* const pTexture, 
				LPCWSTR filePath,
				BOOL bGenerateMipmapIfMissing = FALSE,
				UINT maxSize = 0);

			static BOOL CreateRootSignature(
				Core::Device* const pDevice,
				const D3D12_ROOT_SIGNATURE_DESC& rootSignatureDesc,
				const IID& riid,
				void** const ppRootSignature,
				LPCWSTR name);

			static D3D12_GRAPHICS_PIPELINE_STATE_DESC DefaultPsoDesc(D3D12_INPUT_LAYOUT_DESC inputLayout, DXGI_FORMAT dsvFormat);
			static D3D12_GRAPHICS_PIPELINE_STATE_DESC FitToScreenPsoDesc();

			static D3DX12_MESH_SHADER_PIPELINE_STATE_DESC DefaultMeshPsoDesc(DXGI_FORMAT dsvFormat);

			static BOOL CreateComputePipelineState(
				Core::Device* const pDevice,
				const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc,
				const IID& riid,
				void** const ppPipelineState,
				LPCWSTR name);
			static BOOL CreateGraphicsPipelineState(
				Core::Device* const pDevice,
				const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc,
				const IID& riid,
				void** const ppPipelineState,
				LPCWSTR name);
			static BOOL CreatePipelineState(
				Core::Device* const pDevice,
				const D3DX12_MESH_SHADER_PIPELINE_STATE_DESC& desc,
				const IID& riid,
				void** const ppPipelineState,
				LPCWSTR name);

			static D3D12_INPUT_LAYOUT_DESC InputLayoutDesc();

		private:
			static Common::Debug::LogFile* mpLogFile;
		};
	}
}

#include "D3D12Util.inl"