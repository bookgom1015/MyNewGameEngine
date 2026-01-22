#include "Render/DX/Foundation/Core/pch_d3d12.h"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX/Foundation/Core/Factory.hpp"
#include "Render/DX/Foundation/Resource/GpuResource.hpp"
#include "Render/DX/Foundation/Util/D3D12Util.hpp"

using namespace Render::DX::Foundation::Core;
using namespace Microsoft::WRL;

Device::Device() {}

Device::~Device() { CleanUp(); }

BOOL Device::Initialize(Common::Debug::LogFile* const pLogFile) {
	mpLogFile = pLogFile;

	return TRUE;
}

void Device::CleanUp() {
	if (md3dDevice) md3dDevice.Reset();

	mpLogFile = nullptr;
}

BOOL Device::QueryInterface(Microsoft::WRL::ComPtr<ID3D12InfoQueue1>& pInfoQueue) {
	CheckHRESULT(mpLogFile, md3dDevice->QueryInterface(IID_PPV_ARGS(&pInfoQueue)));

	return TRUE;
}

BOOL Device::CreateCommandQueue(Microsoft::WRL::ComPtr<ID3D12CommandQueue>& pCommandQueue) {
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	CheckHRESULT(mpLogFile, md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&pCommandQueue)));

	return TRUE;
}

BOOL Device::CreateCommandAllocator(Microsoft::WRL::ComPtr<ID3D12CommandAllocator>& pCommandAllocator) {
	CheckHRESULT(mpLogFile, md3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pCommandAllocator)));

	return TRUE;
}

BOOL Device::CreateCommandList(
		ID3D12CommandAllocator* const pCommandAllocator,
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6>& pCommandList) {
	CheckHRESULT(mpLogFile, md3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		pCommandAllocator,	// Associated command allocator
		nullptr,			// Initial PipelineStateObject
		IID_PPV_ARGS(&pCommandList)
	));

	// Start off in a closed state.  This is because the first time we refer 
	// to the command list we will Reset it, and it needs to be closed before
	// calling Reset.
	pCommandList->Close();

	return TRUE;
}

BOOL Device::CreateFence(Microsoft::WRL::ComPtr<ID3D12Fence>& pFence) {
	CheckHRESULT(mpLogFile, md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence)));

	return TRUE;
}

BOOL Device::CreateRtvDescriptorHeap(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& pDescHeap, UINT numDescs) {
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = numDescs;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	CheckHRESULT(mpLogFile, md3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&pDescHeap)));

	return TRUE;
}

BOOL Device::CreateDsvDescriptorHeap(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& pDescHeap, UINT numDescs) {
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = numDescs;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	CheckHRESULT(mpLogFile, md3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&pDescHeap)));

	return TRUE;
}

BOOL Device::CreateCbvUavSrvDescriptorHeap(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& pDescHeap, UINT numDescs) {
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = numDescs;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	CheckHRESULT(mpLogFile, md3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&pDescHeap)));

	return TRUE;
}

UINT Device::DescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType) const {
	return md3dDevice->GetDescriptorHandleIncrementSize(descriptorHeapType);
}

BOOL Device::CheckMeshShaderSupported(BOOL& bMeshShaderSupported) const {
	D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};
	const auto featureSupport = md3dDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7));
	if (FAILED(featureSupport)) {
		std::wstringstream wsstream;
		wsstream << L"CheckFeatureSupport failed: " << std::hex << featureSupport;
		ReturnFalse(mpLogFile, wsstream.str().c_str());
	}
	
	if (options7.MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED) {
		WLogln(mpLogFile, L"Selected device supports mesh shader");
		bMeshShaderSupported = TRUE;
	}
	else {
		WLogln(mpLogFile, L"Selected device does not support mesh shader");
		bMeshShaderSupported = FALSE;
	}

	return TRUE;
}

BOOL Device::CreateRootSignature(
		const D3D12_ROOT_SIGNATURE_DESC& rootSignatureDesc,
		const IID& riid,
		void** const ppRootSignature,
		LPCWSTR name) {
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(
		&rootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(),
		errorBlob.GetAddressOf()
	);

	std::wstringstream wsstream;
	if (errorBlob != nullptr)
		wsstream << reinterpret_cast<char*>(errorBlob->GetBufferPointer());

	if (FAILED(hr))
		ReturnFalse(mpLogFile, wsstream.str().c_str());

	CheckHRESULT(mpLogFile, md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		riid,
		ppRootSignature));

	if (name != nullptr) {
		auto rootSig = reinterpret_cast<ID3D12RootSignature*>(*ppRootSignature);
		rootSig->SetName(name);
	}

	return TRUE;
}

void Device::CreateShaderResourceView(
		ID3D12Resource* const pResource,
		const D3D12_SHADER_RESOURCE_VIEW_DESC* const pDesc,
		D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor) {
	md3dDevice->CreateShaderResourceView(pResource, pDesc, destDescriptor);
}

void Device::CreateUnorderedAccessView(
		ID3D12Resource* const pResource,
		ID3D12Resource* const pCounterResource,
		const D3D12_UNORDERED_ACCESS_VIEW_DESC* const pDesc,
		D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor) {
	md3dDevice->CreateUnorderedAccessView(pResource, pCounterResource, pDesc, destDescriptor);
}

void Device::CreateRenderTargetView(
		ID3D12Resource* const pResource,
		const D3D12_RENDER_TARGET_VIEW_DESC* const pDesc,
		D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor) {
	md3dDevice->CreateRenderTargetView(pResource, pDesc, destDescriptor);
}

void Device::CreateDepthStencilView(
		ID3D12Resource* const pResource,
		const D3D12_DEPTH_STENCIL_VIEW_DESC* const pDesc,
		D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor) {
	md3dDevice->CreateDepthStencilView(pResource, pDesc, destDescriptor);
}

BOOL Device::GetRaytracingAccelerationStructurePrebuildInfo(
		const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS* const pDesc,
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO* pInfo) {
	md3dDevice->GetRaytracingAccelerationStructurePrebuildInfo(pDesc, pInfo);

	return TRUE;
}

BOOL Device::CreateComputePipelineState(
		const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc,
		const IID& riid,
		void** const ppPipelineState,
		LPCWSTR name) {
	CheckHRESULT(mpLogFile, md3dDevice->CreateComputePipelineState(&desc, riid, ppPipelineState));

	if (name != nullptr) {
		auto pso = reinterpret_cast<ID3D12PipelineState*>(*ppPipelineState);
		pso->SetName(name);
	}

	return TRUE;
}

BOOL Device::CreateGraphicsPipelineState(
		const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc,
		const IID& riid,
		void** const ppPipelineState,
		LPCWSTR name) {
	CheckHRESULT(mpLogFile, md3dDevice->CreateGraphicsPipelineState(&desc, riid, ppPipelineState));

	if (name != nullptr) {
		auto pso = reinterpret_cast<ID3D12PipelineState*>(*ppPipelineState);
		pso->SetName(name);
	}

	return TRUE;
}

BOOL Device::CreatePipelineState(
		const D3DX12_MESH_SHADER_PIPELINE_STATE_DESC& desc,
		const IID& riid,
		void** const ppPipelineState,
		LPCWSTR name) {
	auto meshStreamDesc = CD3DX12_PIPELINE_MESH_STATE_STREAM(desc);

	D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
	streamDesc.SizeInBytes = sizeof(meshStreamDesc);
	streamDesc.pPipelineStateSubobjectStream = &meshStreamDesc;

	CheckHRESULT(mpLogFile, md3dDevice->CreatePipelineState(&streamDesc, riid, ppPipelineState));

	if (name != nullptr) {
		auto pso = reinterpret_cast<ID3D12PipelineState*>(*ppPipelineState);
		pso->SetName(name);
	}

	return TRUE;
}

BOOL Device::CreateStateObject(
		const D3D12_STATE_OBJECT_DESC* pDesc,
		const IID& riid,
		void** const ppStateObject) {
	CheckHRESULT(mpLogFile, md3dDevice->CreateStateObject(pDesc, riid, ppStateObject));

	return TRUE;
}
