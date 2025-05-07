#pragma once

#include "Common/ImGuiManager/ImGuiManager.hpp"

#include <memory>

#include <Microsoft.Direct3D.D3D12.1.615.1/build/native/include/d3d12.h>

namespace Common::Foundation::Core {
	class WindowsManager;
}

namespace Render::DX::Foundation {
	namespace Core {
		class Device;
		class DescriptorHeap;
		class CommandObject;
	}

	namespace Resource {
		class FrameResource;
		class GpuResource;
	}
}

namespace ImGuiManager {
	extern "C" ImGuiManagerAPI Common::ImGuiManager::ImGuiManager* CreateImGuiManager();
	extern "C" ImGuiManagerAPI void DestroyImGuiManager(Common::ImGuiManager::ImGuiManager* const imGuiManager);

	namespace DX {
		class DxImGuiManager : public Common::ImGuiManager::ImGuiManager {
		public:
			DxImGuiManager() = default;
			virtual ~DxImGuiManager() = default;

		public:
			BOOL InitializeD3D12(
				Render::DX::Foundation::Core::Device* const pDevice, 
				Render::DX::Foundation::Core::DescriptorHeap* const pDescriptorHeap);
			void CleanUpD3D12();

			void HookMsgCallback(Common::Foundation::Core::WindowsManager* const pWndManager);

			BOOL DrawImGui(
				ID3D12GraphicsCommandList6* const pCmdList,
				UINT clientWidth, UINT clientHeight);

		private:
			BOOL mbIsD3D12Initialized = FALSE;

			D3D12_CPU_DESCRIPTOR_HANDLE mhImGuiCpuSrv;
			D3D12_GPU_DESCRIPTOR_HANDLE mhImGuiGpuSrv;
		};
	}
}