#include "Render/DX/Foundation/Core/Factory.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Foundation/Util/D3D12Util.hpp"

#include <algorithm>

using namespace Microsoft::WRL;
using namespace Render::DX::Foundation::Core;

namespace {
	const WCHAR* const STR_FAIL_NOT_SUPPORT_FEATURES = L" does not support the required features";
	const WCHAR* const STR_FAIL_NOT_SUPPORT_RAYTRACING = L" does not support ray-tracing";
}

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
#if _DEBUG
		std::wstringstream wsstream;
		wsstream << L"    " << desc.Description << L" ( Shared system memory: " << sram << L"MB , Dedicated video memory: " << vram << L"MB )";
#endif
		const UINT score = static_cast<UINT>(desc.DedicatedSystemMemory + desc.DedicatedVideoMemory);

		mAdapters.emplace_back(score, adapter);
		++i;

#if _DEBUG
		WLogln(mpLogFile, wsstream.str());
#endif
	}

	// Sort descending by score
	std::sort(mAdapters.begin(), mAdapters.end(), [](const auto& a, const auto& b) {
		return a.first > b.first;
	});

	return TRUE;
}

BOOL Factory::SelectAdapter(Device* const pDevice) {
	BOOL found = FALSE;
	for (auto begin = mAdapters.begin(), end = mAdapters.end(); begin != end; ++begin) {
		const auto adapter = begin->second;

		// Try to create hardware device.
		const HRESULT hr = D3D12CreateDevice(
			adapter,
			D3D_FEATURE_LEVEL_12_1,
			__uuidof(ID3D12Device5), 
			static_cast<void**>(&pDevice->md3dDevice)
		);
		
		if (SUCCEEDED(hr)) {
			DXGI_ADAPTER_DESC desc;
			adapter->GetDesc(&desc);

			// Checks that device supports ray-tracing.
			D3D12_FEATURE_DATA_D3D12_OPTIONS5 ops = {};
			const auto featureSupport = pDevice->md3dDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &ops, sizeof(ops));
			if (FAILED(featureSupport)) {
				pDevice->md3dDevice = nullptr;
				WLogln(mpLogFile, desc.Description, STR_FAIL_NOT_SUPPORT_FEATURES);
				continue;
			}
			else if (ops.RaytracingTier < D3D12_RAYTRACING_TIER_1_0) {
				pDevice->md3dDevice = nullptr;
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

	return TRUE;
}