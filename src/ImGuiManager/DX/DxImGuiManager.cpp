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
	Texture();
	Inspector();
	Outliner();
	Content();
	Profiler();
	LogUI();

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

void DxImGuiManager::Scene() {
	if (!mbSceneOpened) return;
	
	ImGui::Begin("Scene", &mbSceneOpened);

	ImVec2 avail = ImGui::GetContentRegionAvail();   // 지금 커서 위치에서 남은 영역
	if (avail.x < 1.f) avail.x = 1.f;
	if (avail.y < 1.f) avail.y = 1.f;

	ImGui::Image(
		static_cast<ImTextureID>(mhSceneImageGpuSrv.ptr), avail);

	ImGui::End();
}