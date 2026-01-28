#pragma once

#include "Common/ImGuiManager/ImGuiManager.hpp"

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
			DxImGuiManager();
			virtual ~DxImGuiManager();

		public:
			ImGuiManagerAPI BOOL InitializeD3D12(
				Render::DX::Foundation::Core::Device* const pDevice, 
				Render::DX::Foundation::Core::DescriptorHeap* const pDescriptorHeap);
			ImGuiManagerAPI virtual void CleanUp() override;

			ImGuiManagerAPI BOOL DrawImGui(
				ID3D12GraphicsCommandList6* const pCmdList,
				Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet,
				Common::Foundation::Light* lights[],
				UINT numLights,
				std::queue<std::shared_ptr<Common::Foundation::Light>>& pendingLights,
				UINT clientWidth, UINT clientHeight,
				BOOL bRaytracingSupported);

		private:
			BOOL mbIsD3D12Initialized{};

			D3D12_CPU_DESCRIPTOR_HANDLE mhImGuiCpuSrv{};
			D3D12_GPU_DESCRIPTOR_HANDLE mhImGuiGpuSrv{};
		};
	}
}