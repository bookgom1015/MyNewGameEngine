#include "Render/DX/Foundation/Util/GpuResource.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX/Foundation/Util/D3D12Util.hpp"

using namespace Render::DX::Foundation::Util;

BOOL GpuResource::Initialize(
		Common::Debug::LogFile* const pLogFile,
		ID3D12Device* const pDevice,
		const D3D12_HEAP_PROPERTIES* const pHeapProp,
		D3D12_HEAP_FLAGS heapFlag,
		const D3D12_RESOURCE_DESC* const pRscDesc,
		D3D12_RESOURCE_STATES initialState,
		const D3D12_CLEAR_VALUE* const pOptClear,
		LPCWSTR pName) {
	mpLogFile = pLogFile;

	CheckHRESULT(mpLogFile, pDevice->CreateCommittedResource(
		pHeapProp,
		heapFlag,
		pRscDesc,
		initialState,
		pOptClear,
		IID_PPV_ARGS(&mResource)
	));
	if (pName != nullptr) mResource->SetName(pName);

	mCurrState = initialState;

	return TRUE;
}

BOOL GpuResource::OnResize(IDXGISwapChain* const pSwapChain, UINT index) {
	CheckHRESULT(mpLogFile, pSwapChain->GetBuffer(index, IID_PPV_ARGS(&mResource)));

	mCurrState = D3D12_RESOURCE_STATE_PRESENT;

	return TRUE;
}

void GpuResource::Swap(Microsoft::WRL::ComPtr<ID3D12Resource>& srcResource, D3D12_RESOURCE_STATES initialState) {
	srcResource.Swap(mResource);

	mCurrState = initialState;
}

void GpuResource::Transite(ID3D12GraphicsCommandList* const pCmdList, D3D12_RESOURCE_STATES state) {
	if (mCurrState == state) return;

	pCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mResource.Get(), mCurrState, state));

	if (mCurrState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS || state == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		D3D12Util::UavBarrier(pCmdList, mResource.Get());

	mCurrState = state;
}