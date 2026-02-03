#include "Common/ImGuiManager/pch_imgui_common.h"
#include "Common/ImGuiManager/ImGuiManager.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Core/WindowsManager.hpp"
#include "Common/Foundation/Light.h"
#include "Common/Render/ShadingArgument.hpp"
#include "Common/Render/TonemapperType.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

using namespace Common::ImGuiManager;
using namespace DirectX;

BOOL ImGuiManager::Initialize(Common::Debug::LogFile* const pLogFile, HWND hWnd) {
	mpLogFile = pLogFile;

	// Setup dear ImGui context.
	IMGUI_CHECKVERSION();
	mpContext = ImGui::CreateContext();

	// Setup Dear ImGui style.
	ImGui::StyleColorsDark();

	// Setup platform/renderer backends
	CheckReturn(mpLogFile, ImGui_ImplWin32_Init(hWnd));

	mbIsWin32Initialized = TRUE;

	return TRUE;
}

void ImGuiManager::CleanUp() {
	if (mbIsWin32Initialized) ImGui_ImplWin32_Shutdown();
	if (mpContext != nullptr) ImGui::DestroyContext(mpContext);
}

void ImGuiManager::HookMsgCallback(Common::Foundation::Core::WindowsManager* const pWndManager) {
	pWndManager->HookMsgCallback(ImGui_ImplWin32_WndProcHandler);
}

void ImGuiManager::FrameRateText(UINT clientWidth, UINT clientHeight) {
	CHAR buffer[64];
	snprintf(buffer, sizeof(buffer), "%.1f FPS \n(%.3f ms)", 
		ImGui::GetIO().Framerate, 1000.f / ImGui::GetIO().Framerate);

	const float TextWidth = ImGui::CalcTextSize(buffer).x;
	const float RegionWidth = ImGui::GetContentRegionAvail().x;

	mFrameTimes[mFrameOffset] = 1000.f / ImGui::GetIO().Framerate;
	mFrameOffset = (mFrameOffset + 1) % IM_ARRAYSIZE(mFrameTimes);

	ImGui::PlotLines(
		buffer,
		mFrameTimes,
		IM_ARRAYSIZE(mFrameTimes),
		mFrameOffset,
		nullptr,
		0.0f,
		16.0f,
		ImVec2(0.f, 100.f));
	ImGui::NewLine();
}

void ImGuiManager::RaytraycingEnableCheckBox(Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet) {
	ImGui::Checkbox("Raytracing", reinterpret_cast<bool*>(&pArgSet->RaytracingEnabled));
	ImGui::NewLine();
}

void ImGuiManager::LightHeader(
		Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet,
		Common::Foundation::Light* lights[],
		UINT numLights,
		std::queue<std::shared_ptr<Common::Foundation::Light>>& pendingLights) {
	if (ImGui::CollapsingHeader("Lights")) {
		if (ImGui::Button("Directional")) {
			std::shared_ptr<Common::Foundation::Light> light =
				std::make_shared<Common::Foundation::Light>();
			light->Type = Common::Foundation::LightType::E_Directional;
			light->Direction = { 0.f, -1.f, 0.f };
			light->Color = { 255.f / 255.f, 255.f / 255.f, 255.f / 255.f };
			light->Intensity = 1.f;

			pendingLights.push(light);
		}
		ImGui::SameLine();
		if (ImGui::Button("Spot")) {
			std::shared_ptr<Common::Foundation::Light> light =
				std::make_shared<Common::Foundation::Light>();
			light->Type = Common::Foundation::LightType::E_Spot;
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
			std::shared_ptr<Common::Foundation::Light> light =
				std::make_shared<Common::Foundation::Light>();
			light->Type = Common::Foundation::LightType::E_Point;
			light->Position = { 0.f,0.f,0.f };
			light->Radius = 1.f;
			light->AttenuationRadius = 10.f;
			light->Color = { 255.f / 255.f, 255.f / 255.f, 255.f / 255.f };
			light->Intensity = 1.f;

			pendingLights.push(light);
		}
		ImGui::SameLine();
		if (ImGui::Button("Rect")) {
			std::shared_ptr<Common::Foundation::Light> light =
				std::make_shared<Common::Foundation::Light>();
			light->Type = Common::Foundation::LightType::E_Rect;
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
			std::shared_ptr<Common::Foundation::Light> light = 
				std::make_shared<Common::Foundation::Light>();
			light->Type = Common::Foundation::LightType::E_Tube;
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

			if (light->Type == Common::Foundation::LightType::E_Directional) {
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
			else if (light->Type == Common::Foundation::LightType::E_Spot) {
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
			else if (light->Type == Common::Foundation::LightType::E_Point) {
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
			else if (light->Type == Common::Foundation::LightType::E_Rect) {
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
					ImGui::SliderFloat2("##Light Size", reinterpret_cast<FLOAT*>(&light->Size), 0.f, 10.f);
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
			else if (light->Type == Common::Foundation::LightType::E_Tube) {
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

void ImGuiManager::ShadingObjectHeader(Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet) {
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
		// ChromaticAberration
		ChromaticAberrationTree(pArgSet);
	}
}

void ImGuiManager::ShadowTree(Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet) {
	if (ImGui::TreeNode("Shadow")) {
		ImGui::Checkbox("Enabled", reinterpret_cast<bool*>(&pArgSet->ShadowEnabled));

		ImGui::TreePop();
	}
}

void ImGuiManager::GammaCorrectionTree(Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet) {
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

void ImGuiManager::ToneMappingTree(Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet) {
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

void ImGuiManager::TAATree(Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet) {
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

void ImGuiManager::AOTree(Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet) {
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
				ImGui::SliderInt("##Sample Count", reinterpret_cast<int*>(&pArgSet->SSAO.SampleCount), pArgSet->SSAO.MinSampleCount, pArgSet->SSAO.MaxSampleCount);
			}
		}
		ImGui::Unindent();

		ImGui::TreePop();
	}
}

void ImGuiManager::VolumetricLightTree(Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet) {
	if (ImGui::TreeNode("Volumetric Light")) {
		ImGui::Checkbox("Tricubic Sampling", reinterpret_cast<bool*>(&pArgSet->VolumetricLight.TricubicSamplingEnabled));

		ImGui::Text("Anisotropic Coefficient");
		ImGui::SliderFloat(
			"##Anisotropic Coefficient",
			&pArgSet->VolumetricLight.AnisotropicCoefficient,
			pArgSet->VolumetricLight.MinAnisotropicCoefficient,
			pArgSet->VolumetricLight.MaxAnisotropicCoefficient);

		ImGui::Text("Uniform Density");
		ImGui::SliderFloat(
			"##Uniform Density",
			&pArgSet->VolumetricLight.UniformDensity,
			pArgSet->VolumetricLight.MinUniformDensity,
			pArgSet->VolumetricLight.MaxUniformDensity);

		ImGui::Text("Density Scale");
		ImGui::SliderFloat(
			"##Density Scale",
			&pArgSet->VolumetricLight.DensityScale,
			pArgSet->VolumetricLight.MinDensityScale,
			pArgSet->VolumetricLight.MaxDensityScale);

		ImGui::TreePop();
	}
}

void ImGuiManager::SSCSTree(
	Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet) {
	if (ImGui::TreeNode("SSCS")) {
		ImGui::Checkbox("Enabled", reinterpret_cast<bool*>(&pArgSet->SSCS.Enabled));

		ImGui::Text("Step Count");
		ImGui::SliderInt(
			"##Step Count",
			reinterpret_cast<int*>(&pArgSet->SSCS.Steps),
			pArgSet->SSCS.MinStep,
			pArgSet->SSCS.MaxStep);

		ImGui::Text("Thickness");
		ImGui::SliderFloat("##Thickness",
			&pArgSet->SSCS.Thcikness,
			pArgSet->SSCS.MinThcikness,
			pArgSet->SSCS.MaxThcikness);

		ImGui::Text("Bias Base");
		ImGui::SliderFloat("##Bias Base",
			&pArgSet->SSCS.BiasBase,
			pArgSet->SSCS.MinBiasBase,
			pArgSet->SSCS.MaxBiasBase);

		ImGui::Text("Bias Slope");
		ImGui::SliderFloat("##Bias Slope",
			&pArgSet->SSCS.BiasSlope,
			pArgSet->SSCS.MinBiasSlope,
			pArgSet->SSCS.MaxBiasSlope);

		ImGui::Text("Depth Epsilon Base");
		ImGui::SliderFloat("##Bias Depth Epsilon Base",
			&pArgSet->SSCS.DepthEpsilonBase,
			pArgSet->SSCS.MinDepthEpsilonBase,
			pArgSet->SSCS.MaxDepthEpsilonBase);

		ImGui::Text("Depth Epsilon Scale");
		ImGui::SliderFloat("##Bias Depth Epsilon Scale",
			&pArgSet->SSCS.DepthEpsilonScale,
			pArgSet->SSCS.MinDepthEpsilonScale,
			pArgSet->SSCS.MaxDepthEpsilonScale);

		ImGui::Text("Max Step Scale Far");
		ImGui::SliderFloat("##Max Step Scale Far",
			&pArgSet->SSCS.StepScaleFar,
			pArgSet->SSCS.MinStepScaleFar,
			pArgSet->SSCS.MaxStepScaleFar);

		ImGui::Text("Max Step Scale Far Dist");
		ImGui::SliderFloat("##Max Step Scale Far Dist",
			&pArgSet->SSCS.StepScaleFarDist,
			pArgSet->SSCS.MinStepScaleFarDist,
			pArgSet->SSCS.MaxStepScaleFarDist);

		ImGui::Text("Max Thickness Far Scale");
		ImGui::SliderFloat("##Max Thickness Far Scale",
			&pArgSet->SSCS.ThicknessFarScale,
			pArgSet->SSCS.MinThicknessFarScale,
			pArgSet->SSCS.MaxThicknessFarScale);

		ImGui::Text("Max Thickness Far Dist");
		ImGui::SliderFloat("##Max Thickness Far Dist",
			&pArgSet->SSCS.ThicknessFarDist,
			pArgSet->SSCS.MinThicknessFarDist,
			pArgSet->SSCS.MaxThicknessFarDist);

		ImGui::TreePop();
	}
}

void ImGuiManager::MotionBlurTree(Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet) {
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

void ImGuiManager::BloomTree(Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet) {
	if (ImGui::TreeNode("Bloom")) {
		ImGui::Checkbox("Enabled", reinterpret_cast<bool*>(&pArgSet->Bloom.Enabled));

		ImGui::TreePop();
	}
}

void ImGuiManager::DOFTree(Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet) {
	if (ImGui::TreeNode("Depth of Field")) {
		ImGui::Checkbox("Enabled", reinterpret_cast<bool*>(&pArgSet->DOF.Enabled));

		ImGui::Text("Focus Range");
		ImGui::SliderFloat("##Focus Range",
			&pArgSet->DOF.FocusRange,
			pArgSet->DOF.MinFocusRange,
			pArgSet->DOF.MaxFocusRange);

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

void ImGuiManager::ChromaticAberrationTree(
	Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet) {
	if (ImGui::TreeNode("Chromatic Aberration")) {
		ImGui::Checkbox("Enabled", reinterpret_cast<bool*>(&pArgSet->ChromaticAberration.Enabled));

		ImGui::Text("Strength");
		ImGui::SliderFloat("##Strength",
			&pArgSet->ChromaticAberration.Strength,
			pArgSet->ChromaticAberration.MinStrength,
			pArgSet->ChromaticAberration.MaxStrength);

		ImGui::Text("Threshold");
		ImGui::SliderFloat("##Threshold",
			&pArgSet->ChromaticAberration.Threshold,
			pArgSet->ChromaticAberration.MinThreshold,
			pArgSet->ChromaticAberration.MaxThreshold);

		ImGui::Text("Feather");
		ImGui::SliderFloat("##Feather",
			&pArgSet->ChromaticAberration.Feather,
			pArgSet->ChromaticAberration.MinFeather,
			pArgSet->ChromaticAberration.MaxFeather);

		ImGui::Text("Exponent");
		ImGui::SliderFloat("##Exponent",
			&pArgSet->ChromaticAberration.Exponent,
			pArgSet->ChromaticAberration.MinExponent,
			pArgSet->ChromaticAberration.MaxExponent);

		ImGui::Text("Max Shift Pixel");
		ImGui::SliderInt("##Max Shift Pixel",
			reinterpret_cast<int*>(&pArgSet->ChromaticAberration.ShiftPx),
			pArgSet->ChromaticAberration.MinShiftPx,
			pArgSet->ChromaticAberration.MaxShiftPx);

		ImGui::TreePop();
	}
}