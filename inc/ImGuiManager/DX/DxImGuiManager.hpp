#pragma once

#include "Common/ImGuiManager/ImGuiManager.hpp"

namespace Common::Foundation::Core {
	class WindowsManager;
}

namespace Render::DX::Foundation {
	struct Light;

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

namespace Common::Render::ShadingArgument {
	struct ShadingArgumentSet;
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
			BOOL InitializeD3D12(
				Render::DX::Foundation::Core::Device* const pDevice, 
				Render::DX::Foundation::Core::DescriptorHeap* const pDescriptorHeap);
			virtual void CleanUp() override;

			void HookMsgCallback(
				Common::Foundation::Core::WindowsManager* const pWndManager);

			BOOL DrawImGui(
				ID3D12GraphicsCommandList6* const pCmdList,
				Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet,
				Render::DX::Foundation::Light* lights[],
				UINT numLights,
				std::queue<std::shared_ptr<Render::DX::Foundation::Light>>& pendingLights,
				UINT clientWidth, UINT clientHeight,
				BOOL bRaytracingSupported);

		private:
			void FrameRateText(UINT clientWidth, UINT clientHeight);
			void RaytraycingEnableCheckBox(
				Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet);
			void LightHeader(
				Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet,
				Render::DX::Foundation::Light* lights[],
				UINT numLights,
				std::queue<std::shared_ptr<Render::DX::Foundation::Light>>& pendingLights);

			void ShadingObjectHeader(
				Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet);
			void ShadowTree(
				Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet);
			void GammaCorrectionTree(
				Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet);
			void ToneMappingTree(
				Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet);
			void TAATree(
				Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet);
			void AOTree(
				Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet);
			void VolumetricLightTree(
				Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet);
			void SSCSTree(
				Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet);
			void MotionBlurTree(
				Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet);
			void BloomTree(
				Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet);
			void DOFTree(
				Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet);
			void ChromaticAberrationTree(
				Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet);

		private:
			BOOL mbIsD3D12Initialized{};

			D3D12_CPU_DESCRIPTOR_HANDLE mhImGuiCpuSrv{};
			D3D12_GPU_DESCRIPTOR_HANDLE mhImGuiGpuSrv{};
		};
	}
}