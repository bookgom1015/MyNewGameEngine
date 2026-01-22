#pragma once

#include <Microsoft.Direct3D.D3D12.1.615.1/build/native/include/d3dx12/d3dx12.h>

namespace Common::Debug {
	struct LogFile;
}

namespace ImGuiManager::DX {
	class DxImGuiManager;
}

namespace Render::DX {
	namespace Foundation {
		namespace Resource {
			class GpuResource;
		}

		namespace Util {
			class D3D12Util;
		}

		namespace Core {
			class Factory;

			class Device {
			private:
				friend class Factory;
				friend class Resource::GpuResource;
				friend class Util::D3D12Util;
				friend class ImGuiManager::DX::DxImGuiManager;

			public:
				Device();
				virtual ~Device();

			public:
				BOOL Initialize(Common::Debug::LogFile* const pLogFile);
				void CleanUp();

			public:
				BOOL QueryInterface(Microsoft::WRL::ComPtr<ID3D12InfoQueue1>& pInfoQueue);

				BOOL CreateCommandQueue(Microsoft::WRL::ComPtr<ID3D12CommandQueue>& pCommandQueue);
				BOOL CreateCommandAllocator(Microsoft::WRL::ComPtr<ID3D12CommandAllocator>& pCommandAllocator);
				BOOL CreateCommandList(
					ID3D12CommandAllocator* const pCommandAllocator,
					Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6>& pCommandList);
				BOOL CreateFence(Microsoft::WRL::ComPtr<ID3D12Fence>& pFence);

				BOOL CreateRtvDescriptorHeap(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& pDescHeap, UINT numDescs);
				BOOL CreateDsvDescriptorHeap(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& pDescHeap, UINT numDescs);
				BOOL CreateCbvUavSrvDescriptorHeap(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& pDescHeap, UINT numDescs);
				UINT DescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType) const;

				BOOL CheckMeshShaderSupported(BOOL& bMeshShaderSupported) const;

				BOOL CreateRootSignature(
					const D3D12_ROOT_SIGNATURE_DESC& rootSignatureDesc,
					const IID& riid,
					void** const ppRootSignature,
					LPCWSTR name);

				void CreateShaderResourceView(
					ID3D12Resource* const pResource,
					const D3D12_SHADER_RESOURCE_VIEW_DESC* const pDesc,
					D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor);
				void CreateUnorderedAccessView(
					ID3D12Resource* const pResource,
					ID3D12Resource* const pCounterResource,
					const D3D12_UNORDERED_ACCESS_VIEW_DESC* const pDesc,
					D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor);
				void CreateRenderTargetView(
					ID3D12Resource* const pResource,
					const D3D12_RENDER_TARGET_VIEW_DESC* const pDesc,
					D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor);
				void CreateDepthStencilView(
					ID3D12Resource* const pResource,
					const D3D12_DEPTH_STENCIL_VIEW_DESC* const pDesc,
					D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor);

				BOOL GetRaytracingAccelerationStructurePrebuildInfo(
					const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS* const pDesc,
					D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO* pInfo);

				BOOL CreateComputePipelineState(
					const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc,
					const IID& riid,
					void** const ppPipelineState,
					LPCWSTR name);
				BOOL CreateGraphicsPipelineState(
					const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc,
					const IID& riid,
					void** const ppPipelineState,
					LPCWSTR name);
				BOOL CreatePipelineState(
					const D3DX12_MESH_SHADER_PIPELINE_STATE_DESC& desc,
					const IID& riid,
					void** const ppPipelineState,
					LPCWSTR name);
				BOOL CreateStateObject(
					const D3D12_STATE_OBJECT_DESC* pDesc,
					const IID& riid,
					void** const ppStateObject);

			private:
				Common::Debug::LogFile* mpLogFile{};

				Microsoft::WRL::ComPtr<ID3D12Device5> md3dDevice{};
			};
		}
	}
}

#include "Render/DX/Foundation/Core/Device.inl"