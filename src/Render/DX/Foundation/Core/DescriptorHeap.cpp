#include "Render/DX/Foundation/Core/DescriptorHeap.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Foundation/Core/SwapChain.hpp"
#include "Render/DX/Foundation/Core/DepthStencilBuffer.hpp"
#include "ImGuiManager/DX/DxImGuiManager.hpp"

using namespace Render::DX::Foundation::Core;

BOOL DescriptorHeap::Initialize(
		Common::Debug::LogFile* const pLogFile,
		Device* const pDevice,
		SwapChain* const pSwapChain,
		DepthStencilBuffer* const pDepthStencilBuffer) {
	mpLogFile = pLogFile;
	mDevice = pDevice;

	mpSwapChain = pSwapChain;
	mpDepthStencilBuffer = pDepthStencilBuffer;

	CheckReturn(mpLogFile, BuildDescriptorSizes());

	return TRUE;
}

BOOL DescriptorHeap::CreateDescriptorHeaps(UINT numCbvSrvUav, UINT numRtv, UINT numDsv) {
	CheckReturn(mpLogFile, mDevice->CreateRtvDescriptorHeap(mRtvHeap, mpSwapChain->RtvDescCount() + mpDepthStencilBuffer->RtvDescCount() + numRtv));	
	CheckReturn(mpLogFile, mDevice->CreateDsvDescriptorHeap(mDsvHeap, mpSwapChain->DsvDescCount() + mpDepthStencilBuffer->DsvDescCount() + numDsv));
	CheckReturn(mpLogFile, mDevice->CreateCbvUavSrvDescriptorHeap(mCbvSrvUavHeap, mpSwapChain->CbvSrvUavDescCount() + mpDepthStencilBuffer->CbvSrvUavDescCount() + numCbvSrvUav));

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

BOOL DescriptorHeap::SetDescriptorHeap(ID3D12GraphicsCommandList4* const pCmdList) {
	const auto pDescHeap = mCbvSrvUavHeap.Get();
	ID3D12DescriptorHeap* const descriptorHeaps[] = { pDescHeap };
	pCmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	return TRUE;
}

BOOL DescriptorHeap::BuildDescriptorSizes() {
	mRtvDescriptorSize = mDevice->DescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescriptorSize = mDevice->DescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCbvSrvUavDescriptorSize = mDevice->DescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	return TRUE;
}