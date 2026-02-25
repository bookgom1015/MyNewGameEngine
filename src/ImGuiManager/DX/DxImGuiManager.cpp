#include "ImGuiManager/DX/pch_imgui_dx.h"
#include "ImGuiManager/DX/DxImGuiManager.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Light.h"
#include "Common/Render/ShadingArgument.hpp"
#include "Common/Render/TonemapperType.h"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Foundation/Core/CommandObject.hpp"
#include "Render/DX/Foundation/Core/DescriptorHeap.hpp"
#include "Render/DX/Foundation/Core/SwapChain.hpp"
#include "Render/DX/Foundation/Resource/FrameResource.hpp"
#include "Render/DX/Foundation/Resource/GpuResource.hpp"

using namespace ImGuiManager::DX;
using namespace DirectX;

extern "C" ImGuiManagerAPI Common::ImGuiManager::ImGuiManager* ImGuiManager::CreateImGuiManager() {
	return new DxImGuiManager();
}

extern "C" ImGuiManagerAPI void ImGuiManager::DestroyImGuiManager(Common::ImGuiManager::ImGuiManager* const imGuiManager) {
	delete imGuiManager;
}

DxImGuiManager::DxImGuiManager() {}

DxImGuiManager::~DxImGuiManager() {}

BOOL DxImGuiManager::InitializeD3D12(
		Render::DX::Foundation::Core::Device* const pDevice, 
		Render::DX::Foundation::Core::DescriptorHeap* const pDescriptorHeap) {
	//ImGui_ImplDX12_InitInfo init_info = {};
	//init_info.Device = pDevice->md3dDevice.Get();
	//init_info.CommandQueue = g_pd3dCommandQueue;
	//init_info.NumFramesInFlight = APP_NUM_FRAMES_IN_FLIGHT;
	//init_info.RTVFormat = ShadingConvention::SwapChain::BackBufferFormat;
	//init_info.DSVFormat = ShadingConvention::DepthStencilBuffer::DepthStencilBufferFormat;
	//// Allocating SRV descriptors (for textures) is up to the application, so we provide callbacks.
	//// (current version of the backend will only allocate one descriptor, future versions will need to allocate more)
	//init_info.SrvDescriptorHeap = pDescriptorHeap->mCbvSrvUavHeap.Get();
	//init_info.SrvDescriptorAllocFn = 
	//	[](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) { return pDescriptorHeap->mCbvSrvUavHeap.Get().Alloc(out_cpu_handle, out_gpu_handle); };
	//init_info.SrvDescriptorFreeFn = 
	//	[](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) { return g_pd3dSrvDescHeapAlloc.Free(cpu_handle, gpu_handle); };
	//ImGui_ImplDX12_Init(&init_info);


	mhImGuiCpuSrv = pDescriptorHeap->CbvSrvUavCpuOffset(1);
	mhImGuiGpuSrv = pDescriptorHeap->CbvSrvUavGpuOffset(1);
	
	CheckReturn(mpLogFile, ImGui_ImplDX12_Init(
		pDevice->md3dDevice.Get(),
		Render::DX::Foundation::Core::SwapChain::SwapChainBufferCount,
		ShadingConvention::SwapChain::BackBufferFormat,
		pDescriptorHeap->mCbvSrvUavHeap.Get(),
		mhImGuiCpuSrv,
		mhImGuiGpuSrv));

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.Fonts->AddFontDefault();
	io.Fonts->Build();

	mbIsD3D12Initialized = TRUE;

	return TRUE;
}

void DxImGuiManager::CleanUp() {	
	if (mbIsD3D12Initialized) ImGui_ImplDX12_Shutdown();
	ImGuiManager::CleanUp();
}

BOOL DxImGuiManager::DrawImGui(
		ID3D12GraphicsCommandList6* const pCmdList,
		Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet,
		Common::Foundation::Light* lights[],
		UINT numLights,
		std::queue<std::shared_ptr<Common::Foundation::Light>>& pendingLights,
		UINT clientWidth, UINT clientHeight,
		BOOL bRaytracingSupported) {
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::DockSpaceOverViewport(
		0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

	MenuBar(pArgSet);
	Scene();
	Inspector();
	Outliner();
	Content();

	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), pCmdList);

	// Update and Render additional Platform Windows
	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

	return TRUE;
}

void DxImGuiManager::SetSceneImage(D3D12_GPU_DESCRIPTOR_HANDLE srv) {
	mhSceneImageGpuSrv = srv;
}

void DxImGuiManager::MenuBar(Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet) {
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Level Save")) {

			}

			if (ImGui::MenuItem("Level Load")) {

			}

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
					reinterpret_cast<INT*>(&pArgSet->ToneMapping.TonemapperType),
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

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}
}

void DxImGuiManager::Scene() {
	ImGui::Begin("Scene");

	ImVec2 avail = ImGui::GetContentRegionAvail();   // 지금 커서 위치에서 남은 영역
	if (avail.x < 1.f) avail.x = 1.f;
	if (avail.y < 1.f) avail.y = 1.f;

	ImGui::Image(
		static_cast<ImTextureID>(mhSceneImageGpuSrv.ptr), avail);

	ImGui::End();
}

void DxImGuiManager::Inspector() {
	ImGui::Begin("Inspector");
	ImGui::End();
}

void DxImGuiManager::Outliner() {
	ImGui::Begin("Outliner");
	ImGui::End();
}

void DxImGuiManager::Content() {
	ImGui::Begin("Content");
	ImGui::End();
}