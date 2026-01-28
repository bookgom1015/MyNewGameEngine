#pragma once

#include "Common/ImGuiManager/ImGuiManager.hpp"

namespace Render::DX11::Foundation::Core {
	class Device;
}

namespace ImGuiManager {
	extern "C" ImGuiManagerAPI Common::ImGuiManager::ImGuiManager* CreateImGuiManager();
	extern "C" ImGuiManagerAPI void DestroyImGuiManager(Common::ImGuiManager::ImGuiManager* const imGuiManager);

	namespace DX11 {
		class Dx11ImGuiManager : public Common::ImGuiManager::ImGuiManager {
		public:
			Dx11ImGuiManager();
			virtual ~Dx11ImGuiManager();

		public:
			ImGuiManagerAPI BOOL InitializeD3D11(
				Render::DX11::Foundation::Core::Device* const pDevice);
			ImGuiManagerAPI virtual void CleanUp() override;

		public:
			ImGuiManagerAPI BOOL DrawImGui(
				Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet,
				Common::Foundation::Light* lights[],
				UINT numLights,
				std::queue<std::shared_ptr<Common::Foundation::Light>>& pendingLights,
				UINT clientWidth, UINT clientHeight);

		private:
			BOOL mbIsD3D11Initialized{};
		};
	}
}