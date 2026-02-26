#include "Common/ImGuiManager/pch_imgui_common.h"
#include "Common/ImGuiManager/ImGuiManager.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Core/WindowsManager.hpp"
#include "Common/Foundation/Light.h"
#include "Common/Render/ShadingArgument.hpp"
#include "Common/Render/TonemapperType.h"

// ImGuiImage
// https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

using namespace Common::ImGuiManager;
using namespace DirectX;

BOOL ImGuiManager::Initialize(Common::Debug::LogFile* const pLogFile, HWND hWnd) {
	mpLogFile = pLogFile;

	// Make process DPI aware and obtain main monitor scale
	ImGui_ImplWin32_EnableDpiAwareness();
	float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	mpContext = ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigWindowsMoveFromTitleBarOnly = true;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
	//io.ConfigViewportsNoAutoMerge = true;
	//io.ConfigViewportsNoTaskBarIcon = true;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup scaling
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowMenuButtonPosition = ImGuiDir_None;
	style.WindowRounding = 0.f;
	style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
	style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)
	io.ConfigDpiScaleFonts = true;          // [Experimental] Automatically overwrite style.FontScaleDpi in Begin() when Monitor DPI changes. This will scale fonts but _NOT_ scale sizes/padding for now.
	io.ConfigDpiScaleViewports = true;      // [Experimental] Scale Dear ImGui and Platform Windows when Monitor DPI changes.

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

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

void ImGuiManager::AddDisplayTexture(const std::string& name, ImTextureID id) {
	mDisplayTextures[name] = id;
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

//void ImGuiManager::AOTree(Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet) {
//	if (ImGui::TreeNode("AO")) {
//		ImGui::Checkbox("Enabled", reinterpret_cast<bool*>(&pArgSet->AOEnabled));
//
//		ImGui::SameLine();
//		ImGui::Text(pArgSet->RaytracingEnabled ? "(RTAO)" : "(SSAO)");
//
//		ImGui::Indent();
//		if (pArgSet->AOEnabled) {
//			if (pArgSet->RaytracingEnabled) {
//				// Ray Sorting
//				{
//					ImGui::Text("Ray Sorting");
//
//					ImGui::Indent();
//					{
//						const BOOL RaySortingDisabled = pArgSet->RTAO.SampleCount > 1;
//						{
//							if (RaySortingDisabled) ImGui::BeginDisabled();
//
//							if (ImGui::Checkbox("Ray Sorting", reinterpret_cast<bool*>(&pArgSet->RTAO.RaySortingEnabled))) {
//								pArgSet->RTAO.RandomFrameSeed = FALSE;
//								pArgSet->RTAO.CheckboardRayGeneration = FALSE;
//							}
//							if (RaySortingDisabled) ImGui::SetItemTooltip("Enabled only when sample count is 1");
//
//							{
//								if (!pArgSet->RTAO.RaySortingEnabled) ImGui::BeginDisabled();
//
//								ImGui::Checkbox("Random Frame Seed", reinterpret_cast<bool*>(&pArgSet->RTAO.RandomFrameSeed));
//								if (!pArgSet->RTAO.RaySortingEnabled) ImGui::SetItemTooltip("Enabled only when ray sorting is enabled");
//								ImGui::Checkbox("Checkboard Ray Generation", reinterpret_cast<bool*>(&pArgSet->RTAO.CheckboardRayGeneration));
//								if (!pArgSet->RTAO.RaySortingEnabled) ImGui::SetItemTooltip("Enabled only when ray sorting is enabled");
//
//								if (!pArgSet->RTAO.RaySortingEnabled) ImGui::EndDisabled();
//							}
//
//							if (RaySortingDisabled) ImGui::EndDisabled();
//						}
//
//						ImGui::Text("Sample Set Size");
//						if (ImGui::SliderInt(
//							"##Sample Set Size",
//							reinterpret_cast<int*>(&pArgSet->RTAO.SampleSetSize),
//							static_cast<int>(pArgSet->RTAO.MinSampleSetSize),
//							pArgSet->RTAO.SampleCount > 1 ? 1 : static_cast<int>(pArgSet->RTAO.MaxSampleSetSize))) {
//							if (pArgSet->RTAO.SampleCount > 1) pArgSet->RTAO.SampleSetSize = 1;
//						}
//					}
//					ImGui::Unindent();
//				}
//				// Denoiser
//				{
//					ImGui::Text("Denoiser");
//
//					ImGui::Indent();
//					{
//						ImGui::Checkbox("Fullscreen Blur", reinterpret_cast<bool*>(&pArgSet->RTAO.Denoiser.FullscreenBlurEnabaled));
//						ImGui::Checkbox("Low TSPP Blur", reinterpret_cast<bool*>(&pArgSet->RTAO.Denoiser.DisocclusionBlurEnabled));
//					}
//					ImGui::Unindent();
//				}
//				// AO
//				{
//					ImGui::Text("AO Ray");
//
//					ImGui::Indent();
//					{
//						ImGui::Text("Occlusion Radius");
//						ImGui::SliderFloat("##Occlusion Radius", &pArgSet->RTAO.OcclusionRadius, 0.01f, 32.f);
//
//						ImGui::Text("Occlusion Fade Start");
//						ImGui::SliderFloat("##Occlusion Fade Start", &pArgSet->RTAO.OcclusionFadeStart, 0.f, 32.f);
//
//						ImGui::Text("Occlusion Fade End");
//						ImGui::SliderFloat("##Occlusion Fade End", &pArgSet->RTAO.OcclusionFadeEnd, 0.f, 32.f);
//
//						ImGui::Text("Sample Count");
//						if (ImGui::SliderInt(
//							"##Sample Count",
//							reinterpret_cast<int*>(&pArgSet->RTAO.SampleCount),
//							static_cast<int>(pArgSet->RTAO.MinSampleCount),
//							static_cast<int>(pArgSet->RTAO.MaxSampleCount))) {
//							if (pArgSet->RTAO.SampleCount > 1) {
//								pArgSet->RTAO.RaySortingEnabled = FALSE;
//								pArgSet->RTAO.CheckboardRayGeneration = FALSE;
//								pArgSet->RTAO.SampleSetSize = 1;
//							}
//						}
//					}
//					ImGui::Unindent();
//				}
//			}
//			else {
//				ImGui::Text("Occlusion Radius");
//				ImGui::SliderFloat("##Occlusion Radius", &pArgSet->SSAO.OcclusionRadius, pArgSet->SSAO.MinOcclusionRadius, pArgSet->SSAO.MaxOcclusionRadius);
//
//				ImGui::Text("Occlusion Fade Start");
//				ImGui::SliderFloat("##Occlusion Fade Start", &pArgSet->SSAO.OcclusionFadeStart, pArgSet->SSAO.MinOcclusionFadeStart, pArgSet->SSAO.OcclusionFadeEnd);
//
//				ImGui::Text("Occlusion Fade End");
//				ImGui::SliderFloat("##Occlusion Fade End", &pArgSet->SSAO.OcclusionFadeEnd, pArgSet->SSAO.OcclusionFadeStart, pArgSet->SSAO.MaxOcclusionFadeEnd);
//
//				ImGui::Text("Occlusion Strength");
//				ImGui::SliderFloat("##Occlusion Strength", &pArgSet->SSAO.OcclusionStrength, pArgSet->SSAO.MinOcclusionStrength, pArgSet->SSAO.MaxOcclusionStrength);
//
//				ImGui::Text("Sample Count");
//				ImGui::SliderInt("##Sample Count", reinterpret_cast<int*>(&pArgSet->SSAO.SampleCount), pArgSet->SSAO.MinSampleCount, pArgSet->SSAO.MaxSampleCount);
//			}
//		}
//		ImGui::Unindent();
//
//		ImGui::TreePop();
//	}
//}

//void ImGuiManager::SSCSTree(
//	Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet) {
//	if (ImGui::TreeNode("SSCS")) {
//		ImGui::Checkbox("Enabled", reinterpret_cast<bool*>(&pArgSet->SSCS.Enabled));
//
//		ImGui::Text("Step Count");
//		ImGui::SliderInt(
//			"##Step Count",
//			reinterpret_cast<int*>(&pArgSet->SSCS.Steps),
//			pArgSet->SSCS.MinStep,
//			pArgSet->SSCS.MaxStep);
//
//		ImGui::Text("Thickness");
//		ImGui::SliderFloat("##Thickness",
//			&pArgSet->SSCS.Thcikness,
//			pArgSet->SSCS.MinThcikness,
//			pArgSet->SSCS.MaxThcikness);
//
//		ImGui::Text("Bias Base");
//		ImGui::SliderFloat("##Bias Base",
//			&pArgSet->SSCS.BiasBase,
//			pArgSet->SSCS.MinBiasBase,
//			pArgSet->SSCS.MaxBiasBase);
//
//		ImGui::Text("Bias Slope");
//		ImGui::SliderFloat("##Bias Slope",
//			&pArgSet->SSCS.BiasSlope,
//			pArgSet->SSCS.MinBiasSlope,
//			pArgSet->SSCS.MaxBiasSlope);
//
//		ImGui::Text("Depth Epsilon Base");
//		ImGui::SliderFloat("##Bias Depth Epsilon Base",
//			&pArgSet->SSCS.DepthEpsilonBase,
//			pArgSet->SSCS.MinDepthEpsilonBase,
//			pArgSet->SSCS.MaxDepthEpsilonBase);
//
//		ImGui::Text("Depth Epsilon Scale");
//		ImGui::SliderFloat("##Bias Depth Epsilon Scale",
//			&pArgSet->SSCS.DepthEpsilonScale,
//			pArgSet->SSCS.MinDepthEpsilonScale,
//			pArgSet->SSCS.MaxDepthEpsilonScale);
//
//		ImGui::Text("Max Step Scale Far");
//		ImGui::SliderFloat("##Max Step Scale Far",
//			&pArgSet->SSCS.StepScaleFar,
//			pArgSet->SSCS.MinStepScaleFar,
//			pArgSet->SSCS.MaxStepScaleFar);
//
//		ImGui::Text("Max Step Scale Far Dist");
//		ImGui::SliderFloat("##Max Step Scale Far Dist",
//			&pArgSet->SSCS.StepScaleFarDist,
//			pArgSet->SSCS.MinStepScaleFarDist,
//			pArgSet->SSCS.MaxStepScaleFarDist);
//
//		ImGui::Text("Max Thickness Far Scale");
//		ImGui::SliderFloat("##Max Thickness Far Scale",
//			&pArgSet->SSCS.ThicknessFarScale,
//			pArgSet->SSCS.MinThicknessFarScale,
//			pArgSet->SSCS.MaxThicknessFarScale);
//
//		ImGui::Text("Max Thickness Far Dist");
//		ImGui::SliderFloat("##Max Thickness Far Dist",
//			&pArgSet->SSCS.ThicknessFarDist,
//			pArgSet->SSCS.MinThicknessFarDist,
//			pArgSet->SSCS.MaxThicknessFarDist);
//
//		ImGui::TreePop();
//	}
//}

void ImGuiManager::MarginalSpacing() {
	ImGui::Dummy(ImVec2(0.f, 2.f));
	ImGui::Separator();
	ImGui::Dummy(ImVec2(0.f, 2.f));
}

void ImGuiManager::MenuBar(Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet) {
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Level Save")) {

			}

			if (ImGui::MenuItem("Level Load")) {

			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View")) {
			ImGui::MenuItem("Scene", NULL, &mbSceneOpened);
			ImGui::MenuItem("Texture", NULL, &mbTextureOpened);
			ImGui::MenuItem("Inspector", NULL, &mbInspectorOpened);
			ImGui::MenuItem("Outliner", NULL, &mbOutlinerOpened);
			ImGui::MenuItem("Content", NULL, &mbContentOpened);

			MarginalSpacing();

			ImGui::MenuItem("Profiler", NULL, &mbProfilerOpened);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Render")) {
			ImGui::MenuItem("Raytracing", NULL, &pArgSet->RaytracingEnabled);

			MarginalSpacing();

			if (ImGui::BeginMenu("Shadow")) {
				ImGui::MenuItem("Enable", NULL, &pArgSet->ShadowEnabled);

				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("AO")) {
				ImGui::MenuItem("Enable", NULL, &pArgSet->AOEnabled);

				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Volumetric Light")) {
				ImGui::MenuItem("Enable", NULL, &pArgSet->VolumetricLight.Enabled);

				MarginalSpacing();

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

				ImGui::EndMenu();
			}

			MarginalSpacing();

			if (ImGui::BeginMenu("Gamma Correction")) {
				ImGui::MenuItem("Enable", NULL, &pArgSet->GammaCorrection.Enabled);

				MarginalSpacing();

				if (!pArgSet->GammaCorrection.Enabled) ImGui::BeginDisabled();
				{
					ImGui::Text("Gamma");
					ImGui::SameLine(80);
					ImGui::SliderFloat(
						"##Gamma",
						reinterpret_cast<float*>(&pArgSet->GammaCorrection.Gamma),
						pArgSet->GammaCorrection.MinGamma,
						pArgSet->GammaCorrection.MaxGamma);
				}
				if (!pArgSet->GammaCorrection.Enabled) ImGui::EndDisabled();

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Tone Mapping")) {
				ImGui::Text("Type");
				ImGui::SameLine(100);
				if (ImGui::Combo(
					"##Type",
					reinterpret_cast<int*>(&pArgSet->ToneMapping.TonemapperType),
					Common::Render::TonemapperTypeNames,
					Common::Render::TonemapperType::Count));

				ImGui::Text("Middle Grey");
				ImGui::SameLine(100);
				ImGui::SliderFloat(
					"##Middle Grey",
					reinterpret_cast<float*>(&pArgSet->ToneMapping.MiddleGrayKey),
					pArgSet->ToneMapping.MinMiddleGrayKey, pArgSet->ToneMapping.MaxMiddleGrayKey);

				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("TAA")) {
				ImGui::MenuItem("Enable", NULL, &pArgSet->TAA.Enabled);

				MarginalSpacing();

				ImGui::Text("Modulation Factor");
				ImGui::SliderFloat(
					"##Modulation Factor",
					&pArgSet->TAA.ModulationFactor, 0.f, 1.f);

				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("SSCS")) {
				ImGui::MenuItem("Enable", NULL, &pArgSet->SSCS.Enabled);

				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Bloom")) {
				ImGui::MenuItem("Enable", NULL, &pArgSet->Bloom.Enabled);

				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Chromatric Aberration")) {
				ImGui::MenuItem("Enable", NULL, &pArgSet->ChromaticAberration.Enabled);

				ImGui::Text("Strength");
				ImGui::SameLine(120);
				ImGui::SliderFloat("##Strength",
					&pArgSet->ChromaticAberration.Strength,
					pArgSet->ChromaticAberration.MinStrength,
					pArgSet->ChromaticAberration.MaxStrength);

				ImGui::Text("Threshold");
				ImGui::SameLine(120);
				ImGui::SliderFloat("##Threshold",
					&pArgSet->ChromaticAberration.Threshold,
					pArgSet->ChromaticAberration.MinThreshold,
					pArgSet->ChromaticAberration.MaxThreshold);

				ImGui::Text("Feather");
				ImGui::SameLine(120);
				ImGui::SliderFloat("##Feather",
					&pArgSet->ChromaticAberration.Feather,
					pArgSet->ChromaticAberration.MinFeather,
					pArgSet->ChromaticAberration.MaxFeather);

				ImGui::Text("Exponent");
				ImGui::SameLine(120);
				ImGui::SliderFloat("##Exponent",
					&pArgSet->ChromaticAberration.Exponent,
					pArgSet->ChromaticAberration.MinExponent,
					pArgSet->ChromaticAberration.MaxExponent);

				ImGui::Text("Max Shift Pixel");
				ImGui::SameLine(120);
				ImGui::SliderInt("##Max Shift Pixel",
					reinterpret_cast<int*>(&pArgSet->ChromaticAberration.ShiftPx),
					pArgSet->ChromaticAberration.MinShiftPx,
					pArgSet->ChromaticAberration.MaxShiftPx);

				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Motion Blur")) {
				ImGui::MenuItem("Enable", NULL, &pArgSet->MotionBlur.Enabled);

				MarginalSpacing();

				ImGui::Text("Samples");
				ImGui::SameLine(80);
				ImGui::SliderInt(
					"##Samples",
					reinterpret_cast<int*>(&pArgSet->MotionBlur.SampleCount),
					pArgSet->MotionBlur.MinSampleCount,
					pArgSet->MotionBlur.MaxSampleCount);

				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Depth of Field")) {
				ImGui::MenuItem("Enable", NULL, &pArgSet->DOF.Enabled);

				MarginalSpacing();

				ImGui::Text("Focus Range");
				ImGui::SameLine(140);
				ImGui::SliderFloat("##Focus Range",
					&pArgSet->DOF.FocusRange,
					pArgSet->DOF.MinFocusRange,
					pArgSet->DOF.MaxFocusRange);

				ImGui::Text("Bokeh Samples");
				ImGui::SameLine(140);
				ImGui::SliderInt("##Bokeh #Samples",
					reinterpret_cast<int*>(&pArgSet->DOF.BokehSampleCount),
					pArgSet->DOF.MinBokehSampleCount,
					pArgSet->DOF.MaxBokehSampleCount);

				ImGui::Text("Bokeh Radius");
				ImGui::SameLine(140);
				ImGui::SliderFloat("##Bokeh Radius",
					&pArgSet->DOF.BokehRadius,
					pArgSet->DOF.MinBokehRadius,
					pArgSet->DOF.MaxBokehRadius);

				ImGui::Text("Bokeh Threshold");
				ImGui::SameLine(140);
				ImGui::SliderFloat("##Bokeh Threshold",
					&pArgSet->DOF.BokehThreshold,
					pArgSet->DOF.MinBokehThreshold,
					pArgSet->DOF.MaxBokehThreshold);

				ImGui::Text("Highlight Power");
				ImGui::SameLine(140);
				ImGui::SliderFloat("##Highlight Power",
					&pArgSet->DOF.HighlightPower,
					pArgSet->DOF.MinHighlightPower,
					pArgSet->DOF.MaxHighlightPower);

				ImGui::EndMenu();
			}

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

}

void ImGuiManager::Texture() {
	ImGui::Begin("Texture", &mbTextureOpened);

	ImVec2 avail = ImGui::GetContentRegionAvail();   // 지금 커서 위치에서 남은 영역
	if (avail.x < 1.f) avail.x = 1.f;
	if (avail.y < 1.f) avail.y = 1.f;
		
	std::vector<const char*> keys{};
	keys.reserve(mDisplayTextures.size());

	for (const auto& key : mDisplayTextures | std::views::keys)
		keys.push_back(key.c_str());

	if (ImGui::BeginPopupContextWindow()) {		
		ImGui::Text("Select Texture");

		MarginalSpacing();

		ImGui::Combo(
			"##Textures",
			&mSelectedTexture,
			keys.data(),
			keys.size());

		ImGui::EndPopup();	
	}

	if (mSelectedTexture != -1) 
		ImGui::Image(
			mDisplayTextures[keys[mSelectedTexture]], avail);

	ImGui::End();
}

void ImGuiManager::Inspector() {
	if (!mbInspectorOpened) return;

	ImGui::Begin("Inspector", &mbInspectorOpened);
	ImGui::End();
}

void ImGuiManager::Outliner() {
	if (!mbOutlinerOpened) return;

	ImGui::Begin("Outliner", &mbOutlinerOpened);
	ImGui::End();
}

void ImGuiManager::Content() {
	if (!mbContentOpened) return;

	ImGui::Begin("Content", &mbContentOpened);
	ImGui::End();
}

void ImGuiManager::Profiler() {
	if (!mbProfilerOpened) return;

	ImGui::Begin("Profiler", &mbProfilerOpened, ImGuiWindowFlags_NoDocking);

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

	ImGui::End();
}