#include "Render/DX/Foundation/Core/DescriptorHeap.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX/Foundation/Core/SwapChain.hpp"
#include "Render/DX/Foundation/Core/DepthStencilBuffer.hpp"

using namespace Render::DX::Foundation::Core;

BOOL DescriptorHeap::Initialize(
		Common::Debug::LogFile* const pLogFile,
		ID3D12Device5* const pDevice,
		SwapChain* const pSwapChain,
		DepthStencilBuffer* const pDepthStencilBuffer) {
	mpLogFile = pLogFile;
	md3dDevice = pDevice;

	mpSwapChain = pSwapChain;
	mpDepthStencilBuffer = pDepthStencilBuffer;

	CheckReturn(mpLogFile, BuildDescriptorSizes());

	return TRUE;
}

BOOL DescriptorHeap::CreateDescriptorHeaps(UINT numCbvSrvUav, UINT numRtv, UINT numDsv) {
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = mpSwapChain->RtvDescCount() + mpDepthStencilBuffer->RtvDescCount() + numRtv;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	CheckHRESULT(mpLogFile, md3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRtvHeap)));

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = mpSwapChain->DsvDescCount() + mpDepthStencilBuffer->DsvDescCount() + numDsv;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	CheckHRESULT(mpLogFile, md3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&mDsvHeap)));

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = mpSwapChain->CbvSrvUavDescCount() + mpDepthStencilBuffer->CbvSrvUavDescCount() + numCbvSrvUav;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	CheckHRESULT(mpLogFile, md3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mCbvSrvUavHeap)));

	return TRUE;
}

BOOL DescriptorHeap::BuildDescriptors() {
	const auto cpuStart = mCbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart();
	const auto gpuStart = mCbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart();
	const auto rtvCpuStart = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
	const auto dsvCpuStart = mDsvHeap->GetCPUDescriptorHandleForHeapStart();

	mhCpuCbvSrvUav = CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart);
	mhGpuCbvSrvUav = CD3DX12_GPU_DESCRIPTOR_HANDLE(gpuStart);
	mhCpuRtv = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvCpuStart);
	mhCpuDsv = CD3DX12_CPU_DESCRIPTOR_HANDLE(dsvCpuStart);

	return TRUE;
}

BOOL DescriptorHeap::BuildDescriptorSizes() {
	mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	return TRUE;
}