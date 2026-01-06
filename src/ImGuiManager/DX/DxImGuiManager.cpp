#include "ImGuiManager/DX/DxImGuiManager.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Core/WindowsManager.hpp"
#include "Common/Render/ShadingArgument.hpp"
#include "Common/Render/LightType.h"
#include "Common/Render/TonemapperType.h"
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
		Render::DX::Foundation::Light* lights[],
		UINT numLights,
		std::queue<std::shared_ptr<Render::DX::Foundation::Light>>& pendingLights,
		UINT clientWidth, UINT clientHeight,
		BOOL bRaytracingSupported) {
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	const auto Width = static_cast<FLOAT>(clientWidth);
	const auto Height = static_cast<FLOAT>(clientHeight);

	// Setup window properties
	{
		ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiCond_Once);

		const ImVec2 MinSize(300.f, Height);
		const ImVec2 MaxSize(std::max(Width, 300.f), Height);
		ImGui::SetNextWindowSizeConstraints(MinSize, MaxSize);

		//ImGui::SetNextWindowSize(ImVec2(450.f, static_cast<FLOAT>(clientHeight)), ImGuiCond_Always);
		ImGui::SetNextWindowCollapsed(TRUE, ImGuiCond_Once);
	}
	// Create new window
	{
		ImGui::Begin("Control Panel", nullptr, ImGuiWindowFlags_NoMove); // ImGuiWindowFlags_NoResize

		// Framerate text
		FrameRateText(clientWidth, clientHeight);
		if (bRaytracingSupported) RaytraycingEnableCheckBox(pArgSet);
		// Lights
		LightHeader(pArgSet, lights, numLights, pendingLights);
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
		Render::DX::Foundation::Light* lights[],
		UINT numLights,
		std::queue<std::shared_ptr<Render::DX::Foundation::Light>>& pendingLights) {
	if (ImGui::CollapsingHeader("Lights")) {
		if (ImGui::Button("Directional")) {
			std::shared_ptr<Render::DX::Foundation::Light> light = std::make_shared<Render::DX::Foundation::Light>();
			light->Type = Common::Render::LightType::E_Directional;
			light->Direction = { 0.f, -1.f, 0.f };
			light->Color = { 255.f / 255.f, 255.f / 255.f, 255.f / 255.f };
			light->Intensity = 1.f;

			pendingLights.push(light);
		}
		ImGui::SameLine();
		if (ImGui::Button("Spot")) {
			std::shared_ptr<Render::DX::Foundation::Light> light = std::make_shared<Render::DX::Foundation::Light>();
			light->Type = Common::Render::LightType::E_Spot;
			light->Direction = { 0.f, 0.f, 1.f };
			light->Color = { 255.f / 255.f, 255.f / 255.f, 255.f / 255.f };
			light->Intensity = 1.f;
			light->InnerConeAngle = 30.f;
			light->OuterConeAngle = 45.f;
			light->AttenuationRadius = 10.f;

			pendingLights.push(light);
		}
		ImGui::SameLine();
		if (ImGui::Button("Point")) {
			std::shared_ptr<Render::DX::Foundation::Light> light = std::make_shared<Render::DX::Foundation::Light>();
			light->Type = Common::Render::LightType::E_Point;
			light->Position = { 0.f,0.f,0.f };
			light->Radius = 1.f;
			light->AttenuationRadius = 10.f;
			light->Color = { 255.f / 255.f, 255.f / 255.f, 255.f / 255.f };
			light->Intensity = 1.f;

			pendingLights.push(light);
		}
		ImGui::SameLine();
		if (ImGui::Button("Rect")) {
			std::shared_ptr<Render::DX::Foundation::Light> light = std::make_shared<Render::DX::Foundation::Light>();
			light->Type = Common::Render::LightType::E_Rect;
			light->Center = { 0.f, 0.f, 0.f };
			light->Direction = { 0.f, -1.f, 0.f };
			light->Size = { 1.f, 1.f };
			light->AttenuationRadius = 10.f;
			light->Color = { 255.f / 255.f, 255.f / 255.f, 255.f / 255.f };
			light->Intensity = 1.f;

			pendingLights.push(light);
		}
		ImGui::SameLine();
		if (ImGui::Button("Tube")) {
			std::shared_ptr<Render::DX::Foundation::Light> light = std::make_shared<Render::DX::Foundation::Light>();
			light->Type = Common::Render::LightType::E_Tube;
			light->Position = { -0.5f, 0.f, 0.f };
			light->Position1 = { 0.5f, 0.f, 0.f };
			light->Radius = 1.f;
			light->AttenuationRadius = 10.f;
			light->Color = { 255.f / 255.f, 255.f / 255.f, 255.f / 255.f };
			light->Intensity = 1.f;

			pendingLights.push(light);
		}

		for (UINT i = 0; i < numLights; ++i) {
			const auto light = lights[i];

			if (light->Type == Common::Render::LightType::E_Directional) {
				if (ImGui::TreeNode((std::to_string(i) + " Directional Light").c_str())) {
					ImGui::Text("Light Color");
					ImGui::ColorPicker3("##Light Color", reinterpret_cast<FLOAT*>(&light->Color));
					ImGui::NewLine();

					ImGui::Text("Light Intensity");
					ImGui::SliderFloat("##Light Intensity", &light->Intensity, 0.f, 100.f);
					ImGui::NewLine();

					ImGui::Text("Light Direction");
					if (ImGui::SliderFloat3("##Light Direction", reinterpret_cast<FLOAT*>(&light->Direction), -1.f, 1.f)) {
						const XMVECTOR Direction = XMLoadFloat3(&light->Direction);
						const XMVECTOR Normalized = XMVector3Normalize(Direction);

						XMStoreFloat3(&light->Direction, Normalized);
					}
					ImGui::NewLine();

					ImGui::TreePop();
				}
			}
			else if (light->Type == Common::Render::LightType::E_Spot) {
				if (ImGui::TreeNode((std::to_string(i) + " Spot Light").c_str())) {
					ImGui::Text("Light Color");
					ImGui::ColorPicker3("##Light Color", reinterpret_cast<FLOAT*>(&light->Color));
					ImGui::NewLine();

					ImGui::Text("Light Intensity");
					ImGui::SliderFloat("##Light Intensity", &light->Intensity, 0.f, 1600.f);
					ImGui::NewLine();

					ImGui::Text("Light Position");
					ImGui::InputFloat3("##Light Position", reinterpret_cast<FLOAT*>(&light->Position));
					ImGui::NewLine();

					ImGui::Text("Light Direction");
					if (ImGui::SliderFloat3("##Light Direction", reinterpret_cast<FLOAT*>(&light->Direction), -1.f, 1.f)) {
						const XMVECTOR Direction = XMLoadFloat3(&light->Direction);
						const XMVECTOR Normalized = XMVector3Normalize(Direction);

						XMStoreFloat3(&light->Direction, Normalized);
					}
					ImGui::NewLine();

					ImGui::Text("Inner Cone Angle");
					ImGui::SliderFloat("##Inner Cone Angle", &light->InnerConeAngle, 0.f, 90.f);
					ImGui::NewLine();

					ImGui::Text("Outer Cone Angle");
					ImGui::SliderFloat("##Outer Cone Angle", &light->OuterConeAngle, 0.f, 90.f);
					ImGui::NewLine();

					ImGui::Text("Attenuation Radius");
					ImGui::SliderFloat("##Attenuation Radius", &light->AttenuationRadius, 0.f, 100.f);
					ImGui::NewLine();

					ImGui::TreePop();
				}
			}
			else if (light->Type == Common::Render::LightType::E_Point) {
				if (ImGui::TreeNode((std::to_string(i) + " Point Light").c_str())) {
					ImGui::Text("Light Color");
					ImGui::ColorPicker3("##Light Color", reinterpret_cast<FLOAT*>(&light->Color));
					ImGui::NewLine();

					ImGui::Text("Light Intensity");
					ImGui::SliderFloat("##Light Intensity", &light->Intensity, 0.f, 1600.f);
					ImGui::NewLine();

					ImGui::Text("Light Position");
					ImGui::InputFloat3("##Light Position", reinterpret_cast<FLOAT*>(&light->Position));
					ImGui::NewLine();

					ImGui::Text("Light Radius");
					ImGui::InputFloat("##Light Radius", &light->Radius, 0.f, 100.f);
					ImGui::NewLine();

					ImGui::Text("Attenuation Radius");
					ImGui::SliderFloat("##Attenuation Radius", &light->AttenuationRadius, 0.f, 100.f);
					ImGui::NewLine();

					ImGui::TreePop();
				}
			}
			else if (light->Type == Common::Render::LightType::E_Rect) {
				if (ImGui::TreeNode((std::to_string(i) + " Rectangle Light").c_str())) {
					ImGui::Text("Light Color");
					ImGui::ColorPicker3("##Light Color", reinterpret_cast<FLOAT*>(&light->Color));
					ImGui::NewLine();

					ImGui::Text("Light Intensity");
					ImGui::SliderFloat("##Light Intensity", &light->Intensity, 0.f, 1600.f);
					ImGui::NewLine();

					ImGui::Text("Light Center");
					ImGui::InputFloat3("##Light Center", reinterpret_cast<FLOAT*>(&light->Center));
					ImGui::NewLine();

					ImGui::Text("Attenuation Radius");
					ImGui::SliderFloat("##Attenuation Radius", &light->AttenuationRadius, 0.f, 100.f);
					ImGui::NewLine();

					ImGui::Text("Light Size");
					ImGui::SliderFloat2("##Light Size", reinterpret_cast<FLOAT*>(&light->Size),0.f, 10.f);
					ImGui::NewLine();

					ImGui::Text("Light Direction");
					if (ImGui::SliderFloat3("##Light Direction", reinterpret_cast<FLOAT*>(&light->Direction), -1.f, 1.f)) {
						const XMVECTOR Direction = XMLoadFloat3(&light->Direction);
						const XMVECTOR Normalized = XMVector3Normalize(Direction);

						XMStoreFloat3(&light->Direction, Normalized);
					}
					ImGui::NewLine();

					ImGui::TreePop();
				}
			}
			else if (light->Type == Common::Render::LightType::E_Tube) {
				if (ImGui::TreeNode((std::to_string(i) + " Tube Light").c_str())) {
					ImGui::Text("Light Color");
					ImGui::ColorPicker3("##Light Color", reinterpret_cast<FLOAT*>(&light->Color));
					ImGui::NewLine();

					ImGui::Text("Light Intensity");
					ImGui::SliderFloat("##Light Intensity", &light->Intensity, 0.f, 1600.f);
					ImGui::NewLine();

					ImGui::Text("Light Position 1");
					ImGui::InputFloat3("##Light Position 1", reinterpret_cast<FLOAT*>(&light->Position));
					ImGui::NewLine();

					ImGui::Text("Light Position 2");
					ImGui::InputFloat3("##Light Position 2", reinterpret_cast<FLOAT*>(&light->Position1));
					ImGui::NewLine();

					ImGui::Text("Light Radius");
					ImGui::InputFloat("##Light Radius", &light->Radius, 0.f, 100.f);
					ImGui::NewLine();

					ImGui::Text("Attenuation Radius");
					ImGui::SliderFloat("##Attenuation Radius", &light->AttenuationRadius, 0.f, 100.f);
					ImGui::NewLine();

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
		// GammaCorrection
		GammaCorrectionTree(pArgSet);
		//ToneMapping
		ToneMappingTree(pArgSet);
		// AO
		AOTree(pArgSet);
		// VolumetricLight
		VolumetricLightTree(pArgSet);
		// SSCS
		SSCSTree(pArgSet);
		// MotionBlur
		MotionBlurTree(pArgSet);
		// Bloom
		BloomTree(pArgSet);
		// DOF
		DOFTree(pArgSet);
	}
}

void DxImGuiManager::ShadowTree(Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet) {
	if (ImGui::TreeNode("Shadow")) {
		ImGui::Checkbox("Enabled", reinterpret_cast<bool*>(&pArgSet->ShadowEnabled));

		ImGui::TreePop();
	}
}

void DxImGuiManager::GammaCorrectionTree(Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet) {
	if (ImGui::TreeNode("Gamma Correction")) {
		ImGui::Checkbox("Enabled", reinterpret_cast<bool*>(&pArgSet->GammaCorrection.Enabled));
		
		if (pArgSet->GammaCorrection.Enabled) {
			ImGui::Indent();
			{
				ImGui::SliderFloat("Gamma", reinterpret_cast<float*>(&pArgSet->GammaCorrection.Gamma), pArgSet->GammaCorrection.MinGamma, pArgSet->GammaCorrection.MaxGamma);
			}
			ImGui::Unindent();
		}

		ImGui::TreePop();
	}
}

void DxImGuiManager::ToneMappingTree(Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet) {
	if (ImGui::TreeNode("Tone Mapping")) {
		ImGui::SliderFloat(
			"Exposure", 
			reinterpret_cast<float*>(&pArgSet->ToneMapping.Exposure), 
			pArgSet->ToneMapping.MinExposure, pArgSet->ToneMapping.MaxExposure);
		ImGui::SliderFloat(
			"Middle Gray Key", 
			reinterpret_cast<float*>(&pArgSet->ToneMapping.MiddleGrayKey), 
			pArgSet->ToneMapping.MinMiddleGrayKey, pArgSet->ToneMapping.MaxMiddleGrayKey);

		ImGui::Text("Tonemapper");
		ImGui::Indent();
		{
			ImGui::RadioButton("ACES", reinterpret_cast<INT*>(&pArgSet->ToneMapping.TonemapperType),
				Common::Render::TonemapperType::E_ACES); ImGui::SameLine();
			ImGui::RadioButton("Exponential", reinterpret_cast<INT*>(&pArgSet->ToneMapping.TonemapperType),
				Common::Render::TonemapperType::E_Exponential); ImGui::SameLine();
			ImGui::RadioButton("Reinhard", reinterpret_cast<INT*>(&pArgSet->ToneMapping.TonemapperType),
				Common::Render::TonemapperType::E_Reinhard); 
			ImGui::RadioButton("ReinhardExt", reinterpret_cast<INT*>(&pArgSet->ToneMapping.TonemapperType),
				Common::Render::TonemapperType::E_ReinhardExt); ImGui::SameLine();
			ImGui::RadioButton("Uncharted2", reinterpret_cast<INT*>(&pArgSet->ToneMapping.TonemapperType),
				Common::Render::TonemapperType::E_Uncharted2); ImGui::SameLine();			
			ImGui::RadioButton("Log", reinterpret_cast<INT*>(&pArgSet->ToneMapping.TonemapperType),
				Common::Render::TonemapperType::E_Log);
		}
		ImGui::Unindent();

		ImGui::TreePop();
	}
}

void DxImGuiManager::TAATree(Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet) {
	if (ImGui::TreeNode("TAA")) {
		ImGui::Checkbox("Enabled", reinterpret_cast<bool*>(&pArgSet->TAA.Enabled));
		
		if (pArgSet->TAA.Enabled) {
			ImGui::Indent();
			{
				ImGui::Text("Modulation Factor");
				ImGui::SliderFloat("##Modulation Factor", &pArgSet->TAA.ModulationFactor, 0.f, 1.f);
			}
			ImGui::Unindent();
		}

		ImGui::TreePop();
	}
}

void DxImGuiManager::AOTree(Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet) {
	if (ImGui::TreeNode("AO")) {
		ImGui::Checkbox("Enabled", reinterpret_cast<bool*>(&pArgSet->AOEnabled));

		ImGui::SameLine();
		ImGui::Text(pArgSet->RaytracingEnabled ? "(RTAO)" : "(SSAO)");

		ImGui::Indent();
		if (pArgSet->AOEnabled) {
			if (pArgSet->RaytracingEnabled) {
				// Ray Sorting
				{
					ImGui::Text("Ray Sorting");
					
					ImGui::Indent();
					{
						const BOOL RaySortingDisabled = pArgSet->RTAO.SampleCount > 1;
						{
							if (RaySortingDisabled) ImGui::BeginDisabled();

							if (ImGui::Checkbox("Ray Sorting", reinterpret_cast<bool*>(&pArgSet->RTAO.RaySortingEnabled))) {
								pArgSet->RTAO.RandomFrameSeed = FALSE;
								pArgSet->RTAO.CheckboardRayGeneration = FALSE;
							}
							if (RaySortingDisabled) ImGui::SetItemTooltip("Enabled only when sample count is 1");

							{
								if (!pArgSet->RTAO.RaySortingEnabled) ImGui::BeginDisabled();

								ImGui::Checkbox("Random Frame Seed", reinterpret_cast<bool*>(&pArgSet->RTAO.RandomFrameSeed));
								if (!pArgSet->RTAO.RaySortingEnabled) ImGui::SetItemTooltip("Enabled only when ray sorting is enabled");
								ImGui::Checkbox("Checkboard Ray Generation", reinterpret_cast<bool*>(&pArgSet->RTAO.CheckboardRayGeneration));
								if (!pArgSet->RTAO.RaySortingEnabled) ImGui::SetItemTooltip("Enabled only when ray sorting is enabled");

								if (!pArgSet->RTAO.RaySortingEnabled) ImGui::EndDisabled();
							}

							if (RaySortingDisabled) ImGui::EndDisabled();
						}

						ImGui::Text("Sample Set Size");
						if (ImGui::SliderInt(
							"##Sample Set Size",
							reinterpret_cast<int*>(&pArgSet->RTAO.SampleSetSize),
							static_cast<int>(pArgSet->RTAO.MinSampleSetSize),
							pArgSet->RTAO.SampleCount > 1 ? 1 : static_cast<int>(pArgSet->RTAO.MaxSampleSetSize))) {
							if (pArgSet->RTAO.SampleCount > 1) pArgSet->RTAO.SampleSetSize = 1;
						}
					}
					ImGui::Unindent();
				}
				// Denoiser
				{
					ImGui::Text("Denoiser");
					
					ImGui::Indent();
					{
						ImGui::Checkbox("Fullscreen Blur", reinterpret_cast<bool*>(&pArgSet->RTAO.Denoiser.FullscreenBlurEnabaled));
						ImGui::Checkbox("Low TSPP Blur", reinterpret_cast<bool*>(&pArgSet->RTAO.Denoiser.DisocclusionBlurEnabled));
					}
					ImGui::Unindent();
				}
				// AO
				{
					ImGui::Text("AO Ray");

					ImGui::Indent();
					{
						ImGui::Text("Occlusion Radius");
						ImGui::SliderFloat("##Occlusion Radius", &pArgSet->RTAO.OcclusionRadius, 0.01f, 32.f);

						ImGui::Text("Occlusion Fade Start");
						ImGui::SliderFloat("##Occlusion Fade Start", &pArgSet->RTAO.OcclusionFadeStart, 0.f, 32.f);

						ImGui::Text("Occlusion Fade End");
						ImGui::SliderFloat("##Occlusion Fade End", &pArgSet->RTAO.OcclusionFadeEnd, 0.f, 32.f);

						ImGui::Text("Sample Count");
						if (ImGui::SliderInt(
							"##Sample Count",
							reinterpret_cast<int*>(&pArgSet->RTAO.SampleCount),
							static_cast<int>(pArgSet->RTAO.MinSampleCount),
							static_cast<int>(pArgSet->RTAO.MaxSampleCount))) {
							if (pArgSet->RTAO.SampleCount > 1) {
								pArgSet->RTAO.RaySortingEnabled = FALSE;
								pArgSet->RTAO.CheckboardRayGeneration = FALSE;
								pArgSet->RTAO.SampleSetSize = 1;
							}
						}
					}
					ImGui::Unindent();
				}
			}
			else {
				ImGui::Text("Occlusion Radius");
				ImGui::SliderFloat("##Occlusion Radius", &pArgSet->SSAO.OcclusionRadius, pArgSet->SSAO.MinOcclusionRadius, pArgSet->SSAO.MaxOcclusionRadius);

				ImGui::Text("Occlusion Fade Start");
				ImGui::SliderFloat("##Occlusion Fade Start", &pArgSet->SSAO.OcclusionFadeStart, pArgSet->SSAO.MinOcclusionFadeStart, pArgSet->SSAO.OcclusionFadeEnd);

				ImGui::Text("Occlusion Fade End");
				ImGui::SliderFloat("##Occlusion Fade End", &pArgSet->SSAO.OcclusionFadeEnd, pArgSet->SSAO.OcclusionFadeStart, pArgSet->SSAO.MaxOcclusionFadeEnd);

				ImGui::Text("Occlusion Strength");
				ImGui::SliderFloat("##Occlusion Strength", &pArgSet->SSAO.OcclusionStrength, pArgSet->SSAO.MinOcclusionStrength, pArgSet->SSAO.MaxOcclusionStrength);

				ImGui::Text("Sample Count");
				ImGui::SliderInt("##Sample Count", reinterpret_cast<int*>(&pArgSet->SSAO.SampleCount), pArgSet->SSAO.MinSampleCount, pArgSet->SSAO.MaxSampleCount);}
		}
		ImGui::Unindent();

		ImGui::TreePop();
	}
}

void DxImGuiManager::VolumetricLightTree(Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet) {
	if (ImGui::TreeNode("Volumetric Light")) {
		ImGui::Checkbox("Tricubic Sampling", reinterpret_cast<bool*>(&pArgSet->VolumetricLight.TricubicSamplingEnabled));

		ImGui::Text("Anisotropic Coefficient");
		ImGui::SliderFloat(
			"##Anisotropic Coefficient", 
			&pArgSet->VolumetricLight.AnisotropicCoefficient, 
			pArgSet->VolumetricLight.MinAnisotropicCoefficient, 
			pArgSet->VolumetricLight.MaxAnisotropicCoefficient);

		ImGui::Text("Uniform Density");
		ImGui::SliderFloat("##Uniform Density", &pArgSet->VolumetricLight.UniformDensity, pArgSet->VolumetricLight.MinUniformDensity, pArgSet->VolumetricLight.MaxUniformDensity);

		ImGui::Text("Density Scale");
		ImGui::SliderFloat("##Density Scale", &pArgSet->VolumetricLight.DensityScale, pArgSet->VolumetricLight.MinDensityScale, pArgSet->VolumetricLight.MaxDensityScale);

		ImGui::TreePop();
	}
}

void DxImGuiManager::SSCSTree(
		Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet) {
	if (ImGui::TreeNode("SSCS")) {
		ImGui::Checkbox("Enabled", reinterpret_cast<bool*>(&pArgSet->SSCS.Enabled));

		ImGui::TreePop();
	}
}

void DxImGuiManager::MotionBlurTree(Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet) {
	if (ImGui::TreeNode("MotionBlur")) {
		ImGui::Checkbox("Enabled", reinterpret_cast<bool*>(&pArgSet->MotionBlur.Enabled));

		ImGui::Text("Sample Count");
		ImGui::SliderInt(
			"##Sample Count", 
			reinterpret_cast<int*>(&pArgSet->MotionBlur.SampleCount), 
			pArgSet->MotionBlur.MinSampleCount, pArgSet->MotionBlur.MaxSampleCount);

		ImGui::TreePop();
	}
}

void DxImGuiManager::BloomTree(Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet) {
	if (ImGui::TreeNode("Bloom")) {
		ImGui::Checkbox("Enabled", reinterpret_cast<bool*>(&pArgSet->Bloom.Enabled));

		ImGui::TreePop();
	}
}

void DxImGuiManager::DOFTree(Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet) {
	if (ImGui::TreeNode("Depth of Field")) {
		ImGui::Checkbox("Enabled", reinterpret_cast<bool*>(&pArgSet->DOF.Enabled));

		ImGui::Text("Bokeh");
		ImGui::Indent();
		{
			ImGui::Text("Sample Count");
			ImGui::SliderInt("##Sample Count", 
				reinterpret_cast<int*>(&pArgSet->DOF.BokehSampleCount), 
				pArgSet->DOF.MinBokehSampleCount, 
				pArgSet->DOF.MaxBokehSampleCount);

			ImGui::Text("Radius");
			ImGui::SliderFloat("##Radius",
				&pArgSet->DOF.BokehRadius, 
				pArgSet->DOF.MinBokehRadius,
				pArgSet->DOF.MaxBokehRadius);

			ImGui::Text("Threshold");
			ImGui::SliderFloat("##Threshold",
				&pArgSet->DOF.BokehThreshold,
				pArgSet->DOF.MinBokehThreshold,
				pArgSet->DOF.MaxBokehThreshold);

			ImGui::Text("Highlight Power");
			ImGui::SliderFloat("##Highlight Power",
				&pArgSet->DOF.HighlightPower,
				pArgSet->DOF.MinHighlightPower,
				pArgSet->DOF.MaxHighlightPower);
		}
		ImGui::Unindent();

		ImGui::TreePop();
	}
}