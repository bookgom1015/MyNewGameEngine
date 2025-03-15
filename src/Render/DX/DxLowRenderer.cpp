#include "Render/DX/DxLowRenderer.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/HWInfo.hpp"
#include "Render/DX/Foundation/SwapChain.hpp"
#include "Render/DX/Foundation/DepthStencilBuffer.hpp"

#include <algorithm>

using namespace Microsoft::WRL;
using namespace Render::DX;

namespace {
	void D3D12MessageCallback(
			D3D12_MESSAGE_CATEGORY category,
			D3D12_MESSAGE_SEVERITY severity,
			D3D12_MESSAGE_ID id,
			LPCSTR pDescription,
			void* pContext) {
		std::string str(pDescription);

		std::string sevStr;
		switch (severity) {
		case D3D12_MESSAGE_SEVERITY_CORRUPTION:
			sevStr = "Corruption";
			break;
		case D3D12_MESSAGE_SEVERITY_ERROR:
			sevStr = "Error";
			break;
		case D3D12_MESSAGE_SEVERITY_WARNING:
			sevStr = "Warning";
			break;
		case D3D12_MESSAGE_SEVERITY_INFO:
			return;
			sevStr = "Info";
			break;
		case D3D12_MESSAGE_SEVERITY_MESSAGE:
			sevStr = "Message";
			break;
		}

		std::stringstream sstream;
		sstream << '[' << sevStr << "] " << pDescription;

		const auto lowRenderer = reinterpret_cast<DxLowRenderer*>(pContext);
		Logln(lowRenderer->LogFile(), sstream.str());
	}

	const WCHAR* const STR_FAIL_NOT_SUPPORT_FEATURES = L" does not support the required features";
	const WCHAR* const STR_FAIL_NOT_SUPPORT_RAYTRACING = L" does not support ray-tracing";
}

DxLowRenderer::DxLowRenderer() : mpLogFile(nullptr) {
	mProcessor = std::make_unique<Common::Foundation::Processor>();
	mSwapChain = std::make_unique<Foundation::SwapChain>();
	mDepthStencilBuffer = std::make_unique<Foundation::DepthStencilBuffer>();
}

DxLowRenderer::~DxLowRenderer() {}

Common::Debug::LogFile* DxLowRenderer::LogFile() const {
	return mpLogFile;
}

BOOL DxLowRenderer::Initialize(Common::Debug::LogFile* const pLogFile, HWND hWnd, UINT width, UINT height) {
	mhMainWnd = hWnd;
	mpLogFile = pLogFile;

	mClientWidth = width;
	mClientHeight = height;

	CheckReturn(mpLogFile, GetHWInfo());
	CheckReturn(mpLogFile, InitDirect3D(width, height));
	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

void DxLowRenderer::CleanUp() {

}

BOOL DxLowRenderer::GetHWInfo() {
	CheckReturn(mpLogFile, Common::Foundation::HWInfo::GetCoreInfo(mpLogFile, *mProcessor.get()));

	return TRUE;
}

BOOL DxLowRenderer::CreateRtvAndDsvDescriptorHeaps(UINT numCbvSrvUavDescs, UINT numRtvDescs, UINT numDsvDescs) {
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = mSwapChain->RtvDescCount() + numRtvDescs;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	CheckHRESULT(mpLogFile, md3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRtvHeap)));

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = mDepthStencilBuffer->DsvDescCount() + numDsvDescs;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	CheckHRESULT(mpLogFile, md3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&mDsvHeap)));

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = mSwapChain->CbvSrvUavDescCount() + numCbvSrvUavDescs;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	CheckHRESULT(mpLogFile, md3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mCbvSrvUavHeap)));

	return TRUE;
}

BOOL DxLowRenderer::OnResize(UINT width, UINT height) {
	mClientWidth = width;
	mClientHeight = height;

	CheckReturn(mpLogFile, mSwapChain->OnResize(mClientWidth, mClientHeight));
	CheckReturn(mpLogFile, mDepthStencilBuffer->OnResize(mClientWidth, mClientHeight));

	return TRUE;
}

BOOL DxLowRenderer::FlushCommandQueue() {
	return TRUE;
}

BOOL DxLowRenderer::InitDirect3D(UINT width, UINT height) {
#ifdef _DEBUG
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&mDebugController)))) {
		mDebugController->EnableDebugLayer();
		mdxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
	}
#endif

	CheckHRESULT(mpLogFile, CreateDXGIFactory2(mdxgiFactoryFlags, IID_PPV_ARGS(&mDxgiFactory)));

	ComPtr<IDXGIFactory5> factory5;
	CheckHRESULT(mpLogFile, mDxgiFactory.As(&factory5));

	const auto supported = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &mbAllowTearing, sizeof(mbAllowTearing));
	if (SUCCEEDED(supported)) mbAllowTearing = TRUE;

	Adapters adapters;
	SortAdapters(adapters);

	BOOL found = FALSE;
	for (auto begin = adapters.begin(), end = adapters.end(); begin != end; ++begin) {
		const auto adapter = begin->second;

		// Try to create hardware device.
		const HRESULT hr = D3D12CreateDevice(
			adapter,
			D3D_FEATURE_LEVEL_12_1,
			__uuidof(ID3D12Device5),
			static_cast<void**>(&md3dDevice)
		);

		if (SUCCEEDED(hr)) {
			DXGI_ADAPTER_DESC desc;
			adapter->GetDesc(&desc);

			// Checks that device supports ray-tracing.
			D3D12_FEATURE_DATA_D3D12_OPTIONS5 ops = {};
			const auto featureSupport = md3dDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &ops, sizeof(ops));
			if (FAILED(featureSupport)) {
				md3dDevice = nullptr;
				WLogln(mpLogFile, desc.Description, STR_FAIL_NOT_SUPPORT_FEATURES);
				continue;
			}
			else if (ops.RaytracingTier < D3D12_RAYTRACING_TIER_1_0) {
				md3dDevice = nullptr;
				WLogln(mpLogFile, desc.Description, STR_FAIL_NOT_SUPPORT_RAYTRACING);
				continue;
			}

			found = TRUE;
		#ifdef _DEBUG
			WLogln(mpLogFile, desc.Description, L" is selected");
		#endif

			break;
		}
	}
	if (!found) ReturnFalse(mpLogFile, STR_FAIL_NOT_SUPPORT_FEATURES);

	CheckHRESULT(mpLogFile, md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));

	mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

#if _DEBUG
	CheckReturn(mpLogFile, CreateDebugObjects());
#endif
	CheckReturn(mpLogFile, CreateCommandObjects());
	CheckReturn(mpLogFile, CreateSwapChain());
	CheckReturn(mpLogFile, CreateDepthStencilBuffer());
	CheckReturn(mpLogFile, CreateRtvAndDsvDescriptorHeaps(0, 0, 0));

	return TRUE;
}

BOOL DxLowRenderer::SortAdapters(Adapters& adapters) {
	UINT i = 0;
	IDXGIAdapter* adapter = nullptr;

#if _DEBUG
	WLogln(mpLogFile, L"Adapters:");	
#endif

	while (mDxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND) {
		DXGI_ADAPTER_DESC desc;
		if (FAILED(adapter->GetDesc(&desc))) continue;

		const UINT sram = static_cast<UINT>(desc.SharedSystemMemory / (1024 * 1024));
		const UINT vram = static_cast<UINT>(desc.DedicatedVideoMemory / (1024 * 1024));
	#if _DEBUG
		std::wstringstream wsstream;
		wsstream << L"    " << desc.Description << L"( Shared system memory: " << sram << L"MB , Dedicated video memory: " << vram << L"MB )";
	#endif
		const UINT score = static_cast<UINT>(desc.DedicatedSystemMemory + desc.DedicatedVideoMemory);

		adapters.emplace_back(score, adapter);
		++i;

	#if _DEBUG
		WLogln(mpLogFile, wsstream.str());
	#endif
	}

	// Sort descending by score
	std::sort(adapters.begin(), adapters.end(), [](const auto& a, const auto& b) {
		return a.first > b.first;
	});

	return TRUE;
}

BOOL DxLowRenderer::CreateDebugObjects() {
	CheckHRESULT(mpLogFile, md3dDevice->QueryInterface(IID_PPV_ARGS(&mInfoQueue)));
	CheckHRESULT(mpLogFile, mInfoQueue->RegisterMessageCallback(D3D12MessageCallback, D3D12_MESSAGE_CALLBACK_IGNORE_FILTERS, this, &mCallbakCookie));

	return TRUE;
}

BOOL DxLowRenderer::CreateCommandObjects() {
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	CheckHRESULT(mpLogFile, md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

	CheckHRESULT(mpLogFile, md3dDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(mDirectCmdListAlloc.GetAddressOf()))
	);

	CheckHRESULT(mpLogFile, md3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		mDirectCmdListAlloc.Get(),	// Associated command allocator
		nullptr,					// Initial PipelineStateObject
		IID_PPV_ARGS(mDirectCommandList.GetAddressOf())
	));
	// Start off in a closed state.  This is because the first time we refer 
	// to the command list we will Reset it, and it needs to be closed before
	// calling Reset.
	mDirectCommandList->Close();

	mMultiCommandLists.resize(mProcessor->Logical);
	for (UINT i = 0, end = static_cast<UINT>(mProcessor->Logical); i < end; ++i) {
		CheckHRESULT(mpLogFile, md3dDevice->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			mDirectCmdListAlloc.Get(),
			nullptr,
			IID_PPV_ARGS(mMultiCommandLists[i].GetAddressOf())
		));
		mMultiCommandLists[i]->Close();
	}

	return TRUE;
}

BOOL DxLowRenderer::CreateSwapChain() {
	const auto initData = std::make_unique<Foundation::SwapChain::InitData>();
	initData->LogFile = mpLogFile;
	initData->DxgiFactory = mDxgiFactory.Get();
	initData->Device = md3dDevice.Get();
	initData->CommandQueue = mCommandQueue.Get();
	initData->MainWnd = mhMainWnd;
	initData->Width = mClientWidth;
	initData->Height = mClientHeight;
	initData->AllowTearing = mbAllowTearing;

	CheckReturn(mpLogFile, mSwapChain->Initialize(initData.get()));

	return TRUE;
}

BOOL DxLowRenderer::CreateDepthStencilBuffer() {
	const auto initData = std::make_unique<Foundation::DepthStencilBuffer::InitData>();
	initData->LogFile = mpLogFile;
	initData->Width = mClientWidth;
	initData->Height = mClientHeight;
	initData->Device = md3dDevice.Get();

	CheckReturn(mpLogFile, mDepthStencilBuffer->Initialize(initData.get()));

	return TRUE;
}

BOOL DxLowRenderer::BuildDescriptors() {
	const auto cpuStart = mCbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart();
	const auto gpuStart = mCbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart();
	const auto rtvCpuStart = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
	const auto dsvCpuStart = mDsvHeap->GetCPUDescriptorHandleForHeapStart();

	mhCpuCbvSrvUav = CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart);
	mhGpuCbvSrvUav = CD3DX12_GPU_DESCRIPTOR_HANDLE(gpuStart);
	mhCpuRtv = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvCpuStart);
	mhCpuDsv = CD3DX12_CPU_DESCRIPTOR_HANDLE(dsvCpuStart);

	CheckReturn(mpLogFile, mSwapChain->BuildDescriptors(
		mhCpuCbvSrvUav, mhGpuCbvSrvUav, mhCpuRtv, mhCpuDsv, mCbvSrvUavDescriptorSize, mRtvDescriptorSize, mDsvDescriptorSize));
	CheckReturn(mpLogFile, mDepthStencilBuffer->BuildDescriptors(
		mhCpuCbvSrvUav, mhGpuCbvSrvUav, mhCpuRtv, mhCpuDsv, mCbvSrvUavDescriptorSize, mRtvDescriptorSize, mDsvDescriptorSize));

	return TRUE;
}
