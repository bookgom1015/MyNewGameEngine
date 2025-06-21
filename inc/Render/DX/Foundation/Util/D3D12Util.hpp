#pragma once

#include <vector>

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
#include <DirectXTex.h>

#ifndef ReleaseCom
#define ReleaseCom(x) { if (x){ x->Release(); x = NULL; } }
#endif

#ifndef Align
#define Align(alignment, val) (((val + alignment - 1) / alignment) * alignment)
#endif

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
			struct D3D12BufferCreateInfo {
				UINT64					Size		= 0;
				UINT64					Alignment	= 0;
				D3D12_HEAP_TYPE			HeapType	= D3D12_HEAP_TYPE_DEFAULT;
				D3D12_HEAP_FLAGS		HeapFlags	= D3D12_HEAP_FLAG_NONE;
				D3D12_RESOURCE_FLAGS	Flags	= D3D12_RESOURCE_FLAG_NONE;
				D3D12_RESOURCE_STATES	State	= D3D12_RESOURCE_STATE_COMMON;

				D3D12BufferCreateInfo();
				D3D12BufferCreateInfo(UINT64 size, D3D12_RESOURCE_FLAGS flags);
				D3D12BufferCreateInfo(UINT64 size, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES state);
				D3D12BufferCreateInfo(UINT64 size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state);
				D3D12BufferCreateInfo(UINT64 size, UINT64 alignment, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state);
				D3D12BufferCreateInfo(UINT64 size, UINT64 alignment, D3D12_HEAP_TYPE heapType, D3D12_HEAP_FLAGS heapFlags, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state);
			};

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
				const IID& riid,
				void** const ppResource);
			static BOOL CreateBuffer(
				Core::Device* const pDevice, 
				D3D12BufferCreateInfo& info, 
				const IID& riid,
				void** const ppResource,
				ID3D12InfoQueue* pInfoQueue = nullptr);

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
			static void CreateUnorderedAccessView(
				Core::Device* const pDevice,
				ID3D12Resource* const pResource,
				ID3D12Resource* const pCounterResource,
				const D3D12_UNORDERED_ACCESS_VIEW_DESC* const pDesc,
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
			static D3DX12_MESH_SHADER_PIPELINE_STATE_DESC FitToScreenMeshPsoDesc();

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
			static BOOL CreateStateObject(
				Core::Device* const pDevice,
				const D3D12_STATE_OBJECT_DESC* pDesc,
				const IID& riid,
				void** const ppStateObject);

			static D3D12_INPUT_LAYOUT_DESC InputLayoutDesc();

			static BOOL CaptureTexture(
				Core::CommandObject* const pCmdObject,
				ID3D12Resource* const pSource,
				BOOL isCubeMap,
				DirectX::ScratchImage& result,
				D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATES afterState = D3D12_RESOURCE_STATE_RENDER_TARGET);

			template <typename T>
			static void SetRoot32BitConstants(
				UINT RootParameterIndex,
				UINT Num32BitValuesToSet,
				const void* pSrcData,
				UINT DestOffsetIn32BitValues,
				ID3D12GraphicsCommandList6* const pCmdList,
				BOOL isCompute);

		private:
			static Common::Debug::LogFile* mpLogFile;
		};
	}
}

#include "D3D12Util.inl"