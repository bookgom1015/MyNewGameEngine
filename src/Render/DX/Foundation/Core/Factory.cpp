#include "Render/DX/Foundation/Core/pch_d3d12.h"
#include "Render/DX/Foundation/Core/Factory.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Foundation/Util/D3D12Util.hpp"

#include <algorithm>

using namespace Microsoft::WRL;
using namespace Render::DX::Foundation::Core;

Factory::Factory() {}

Factory::~Factory() {}

BOOL Factory::Initialize(Common::Debug::LogFile* const pLogFile) {
	mpLogFile = pLogFile;

	CheckReturn(mpLogFile, CreateFactory());

	return TRUE;
}

BOOL Factory::CreateFactory() {
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

	return TRUE;
}

BOOL Factory::SortAdapters() {
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
		const UINT score = static_cast<UINT>(desc.DedicatedSystemMemory + desc.DedicatedVideoMemory);

		mAdapters.emplace_back(score, adapter);
		++i;

#if _DEBUG
		auto msg = std::format(L"    {} ( Shared system memory: {}MB , Dedicated video memory: {}MB )", 
			desc.Description, sram, vram);
		WLogln(mpLogFile, msg);
#endif
	}

	// Sort descending by score
	std::sort(mAdapters.begin(), mAdapters.end(), [](const auto& a, const auto& b) {
		return a.first > b.first;
	});

	return TRUE;
}

BOOL Factory::GetAdapters(std::vector<std::wstring>& adapters) {
	for (const auto& pair : mAdapters) {
		auto adapter = pair.second;

		DXGI_ADAPTER_DESC desc;
		if (FAILED(adapter->GetDesc(&desc))) continue;
		
		adapters.push_back(desc.Description);
	}

	return TRUE;
}

BOOL Factory::SelectAdapter(Device* const pDevice, UINT adapterIndex, BOOL& bRaytracingSupported) {
	const auto adapter = mAdapters[adapterIndex].second;

	// Try to create hardware device.
	const HRESULT hr = D3D12CreateDevice(
		adapter,
		D3D_FEATURE_LEVEL_12_1,
		__uuidof(ID3D12Device5), 
		static_cast<void**>(&pDevice->md3dDevice)
	);
		
	if (FAILED(hr)) ReturnFalse(mpLogFile, L"Failed to create device");

	DXGI_ADAPTER_DESC desc;
	adapter->GetDesc(&desc);

#ifdef _DEBUG
	WLogln(mpLogFile, desc.Description, L" is selected");
#endif

	D3D12_FEATURE_DATA_D3D12_OPTIONS5 ops = {};
	const auto featureSupport = pDevice->md3dDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &ops, sizeof(ops));
	if (FAILED(featureSupport)) {
		pDevice->md3dDevice = nullptr;
		auto msg = std::format(L"CheckFeatureSupport failed: 0x{:08X}", featureSupport);
		ReturnFalse(mpLogFile, msg);
	}

	if (ops.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0) {
		bRaytracingSupported = TRUE;
		WLogln(mpLogFile, L"Selected device supports ray-tracing");
	}
	else {
		bRaytracingSupported = FALSE;
		WLogln(mpLogFile, L"Selected device does not support ray-tracing");
	}

	return TRUE;
}