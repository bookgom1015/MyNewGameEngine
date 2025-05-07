#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <wrl.h>
#include <Windows.h>

#include <Microsoft.Direct3D.D3D12.1.615.1/build/native/include/d3d12.h>

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
				Device() = default;
				virtual ~Device() = default;

			public:
				BOOL Initialize(Common::Debug::LogFile* const pLogFile);

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

			private:
				Common::Debug::LogFile* mpLogFile = nullptr;

				Microsoft::WRL::ComPtr<ID3D12Device5> md3dDevice;
			};
		}
	}
}

#include "Render/DX/Foundation/Core/Device.inl"