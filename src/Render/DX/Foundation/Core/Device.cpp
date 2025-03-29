#include "Render/DX/Foundation/Core/Device.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX/Foundation/Core/Factory.hpp"
#include "Render/DX/Foundation/Util/GpuResource.hpp"
#include "Render/DX/Foundation/Util/D3D12Util.hpp"

using namespace Render::DX::Foundation::Core;

BOOL Device::Initialize(Common::Debug::LogFile* const pLogFile) {
	mpLogFile = pLogFile;

	return TRUE;
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
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4>& pCommandList) {
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