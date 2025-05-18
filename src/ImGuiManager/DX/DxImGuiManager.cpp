#include "ImGuiManager/DX/DxImGuiManager.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Core/WindowsManager.hpp"
#include "Common/Render/ShadingArgument.hpp"
#include "Common/Render/LightType.h"
#include "Render/DX/Foundation/Light.h"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Foundation/Core/CommandObject.hpp"
#include "Render/DX/Foundation/Core/DescriptorHeap.hpp"
#include "Render/DX/Foundation/Core/SwapChain.hpp"
#include "Render/DX/Foundation/Resource/FrameResource.hpp"
#include "Render/DX/Foundation/Resource/GpuResource.hpp"

#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_dx12.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

using namespace ImGuiManager::DX;
using namespace DirectX;

extern "C" ImGuiManagerAPI Common::ImGuiManager::ImGuiManager* ImGuiManager::CreateImGuiManager() {
	return new DxImGuiManager();
}

extern "C" ImGuiManagerAPI void ImGuiManager::DestroyImGuiManager(Common::ImGuiManager::ImGuiManager* const imGuiManager) {
	delete imGuiManager;
}

BOOL DxImGuiManager::InitializeD3D12(
		Render::DX::Foundation::Core::Device* const pDevice, 
		Render::DX::Foundation::Core::DescriptorHeap* const pDescriptorHeap) {
	ImGui::SetCurrentContext(mpContext);

	mhImGuiCpuSrv = pDescriptorHeap->CbvSrvUavCpuOffset(1);
	mhImGuiGpuSrv = pDescriptorHeap->CbvSrvUavGpuOffset(1);
	
	CheckReturn(mpLogFile, ImGui_ImplDX12_Init(
		pDevice->md3dDevice.Get(),
		Render::DX::Foundation::Core::SwapChain::SwapChainBufferCount,
		ShadingConvention::SwapChain::BackBufferFormat,
		pDescriptorHeap->mCbvSrvUavHeap.Get(),
		mhImGuiCpuSrv,
		mhImGuiGpuSrv));

	mbIsD3D12Initialized = TRUE;

	return TRUE;
}

void DxImGuiManager::CleanUpD3D12() {
	if (mbIsD3D12Initialized) ImGui_ImplDX12_Shutdown();
}

void DxImGuiManager::HookMsgCallback(Common::Foundation::Core::WindowsManager* const pWndManager) {
	pWndManager->HookMsgCallback(ImGui_ImplWin32_WndProcHandler);
}

BOOL DxImGuiManager::DrawImGui(
		ID3D12GraphicsCommandList6* const pCmdList,
		Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet,
		Render::DX::Foundation::Light lights[],
		UINT numLights,
		UINT clientWidth, UINT clientHeight,
		BOOL bRaytracingSupported) {
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// ControlPanel
	{
		ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiCond_Once);
		ImGui::SetNextWindowSize(ImVec2(static_cast<FLOAT>(clientWidth) * 0.5f, static_cast<FLOAT>(clientHeight)), ImGuiCond_Always);
		ImGui::SetNextWindowCollapsed(TRUE, ImGuiCond_Once);
		ImGui::Begin("Control Panel", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

		// Framerate text
		FrameRateText(clientWidth, clientHeight);
		if (bRaytracingSupported) RaytraycingEnableCheckBox(pArgSet);
		// Lights
		LightHeader(pArgSet, lights, numLights);
		// Shading objects
		ShadingObjectHeader(pArgSet);
	
		ImGui::End();
	}

	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), pCmdList);

	return TRUE;
}

void DxImGuiManager::FrameRateText(UINT clientWidth, UINT clientHeight) {
	CHAR buffer[64];
	snprintf(buffer, sizeof(buffer), "%.1f FPS (%.3f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);

	const float TextWidth = ImGui::CalcTextSize(buffer).x;
	const float RegionWidth = ImGui::GetContentRegionAvail().x;

	ImGui::SetCursorPosX(RegionWidth - TextWidth);
	ImGui::TextUnformatted(buffer);
	ImGui::NewLine();
}

void DxImGuiManager::RaytraycingEnableCheckBox(Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet) {
	ImGui::Checkbox("Raytracing", reinterpret_cast<bool*>(&pArgSet->RaytracingEnabled));
	ImGui::NewLine();
}

void DxImGuiManager::LightHeader(
		Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet,
		Render::DX::Foundation::Light lights[],
		UINT numLights) {
	if (ImGui::CollapsingHeader("Lights")) {
		for (UINT i = 0; i < numLights; ++i) {
			auto& light = lights[i];

			if (light.Type == Common::Render::LightType::E_Directional) {
				if (ImGui::TreeNode((std::to_string(i) + " Directional Light").c_str())) {
					ImGui::ColorPicker3("Light Color", reinterpret_cast<FLOAT*>(&light.Color));
					ImGui::SliderFloat("Light Intensity", &light.Intensity, 0.f, 100.f);
					if (ImGui::SliderFloat3("Light Direction", reinterpret_cast<FLOAT*>(&light.Direction), -1.f, 1.f)) {
						const XMVECTOR Direction = XMLoadFloat3(&light.Direction);
						const XMVECTOR Normalized = XMVector3Normalize(Direction);

						XMStoreFloat3(&light.Direction, Normalized);
					}

					ImGui::TreePop();
				}
			}
		}
	}
}

void DxImGuiManager::ShadingObjectHeader(Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet) {
	if (ImGui::CollapsingHeader("Shading Objects")) {
		// Shadow
		ShadowTree(pArgSet);
		// TAA
		TAATree(pArgSet);
		// AO
		AOTree(pArgSet);
	}
}

void DxImGuiManager::ShadowTree(Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet) {
	if (ImGui::TreeNode("Shadow")) {
		ImGui::Checkbox("Enabled", reinterpret_cast<bool*>(&pArgSet->ShadowEnabled));

		ImGui::TreePop();
	}
}

void DxImGuiManager::TAATree(Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet) {
	if (ImGui::TreeNode("TAA")) {
		ImGui::Checkbox("Enabled", reinterpret_cast<bool*>(&pArgSet->TAA.Enabled));
		ImGui::SliderFloat("Modulation Factor", &pArgSet->TAA.ModulationFactor, 0.f, 1.f);

		ImGui::TreePop();
	}
}

void DxImGuiManager::AOTree(Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet) {
	if (ImGui::TreeNode("AO")) {
		ImGui::Checkbox("Enabled", reinterpret_cast<bool*>(&pArgSet->AOEnabled));
		if (pArgSet->AOEnabled) {
			if (pArgSet->RaytracingEnabled) {
				ImGui::Text("RTAO");
				ImGui::SliderFloat("Occlusion Radius", &pArgSet->RTAO.OcclusionRadius, 0.01f, 32.f);
				ImGui::SliderFloat("Occlusion Fade Start", &pArgSet->RTAO.OcclusionFadeStart, 0.f, 32.f);
				ImGui::SliderFloat("Occlusion Fade End", &pArgSet->RTAO.OcclusionFadeEnd, 0.f, 32.f);
				ImGui::SliderInt("Sample Count", reinterpret_cast<int*>(&pArgSet->RTAO.SampleCount), 1, 4);
			}
			else {
				ImGui::Text("SSAO");
				ImGui::SliderFloat("Occlusion Radius", &pArgSet->SSAO.OcclusionRadius, 0.01f, 1.f);
				ImGui::SliderFloat("Occlusion Fade Start", &pArgSet->SSAO.OcclusionFadeStart, 0.f, 10.f);
				ImGui::SliderFloat("Occlusion Fade End", &pArgSet->SSAO.OcclusionFadeEnd, 0.f, 10.f);
				ImGui::SliderFloat("Occlusion Strength", &pArgSet->SSAO.OcclusionStrength, 1.f, 10.f);
				ImGui::SliderInt("Sample Count", reinterpret_cast<int*>(&pArgSet->SSAO.SampleCount), 1, 16);
				ImGui::SliderInt("Blur Count", reinterpret_cast<int*>(&pArgSet->SSAO.BlurCount), 0, 10);
			}
		}

		ImGui::TreePop();
	}
}