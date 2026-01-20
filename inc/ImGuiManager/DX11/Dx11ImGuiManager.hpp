#pragma once

#include "Common/ImGuiManager/ImGuiManager.hpp"

namespace ImGuiManager {
	extern "C" ImGuiManagerAPI Common::ImGuiManager::ImGuiManager* CreateImGuiManager();
	extern "C" ImGuiManagerAPI void DestroyImGuiManager(Common::ImGuiManager::ImGuiManager* const imGuiManager);

	namespace DX11 {
		class Dx11ImGuiManager : public Common::ImGuiManager::ImGuiManager {
		public:
			Dx11ImGuiManager();
			virtual ~Dx11ImGuiManager();

		public:
			BOOL InitializeD3D11();
			void CleanUpD3D11();

		private:
			BOOL mbIsD3D11Initialized{};
		};
	}
}