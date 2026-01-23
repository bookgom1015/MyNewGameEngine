#include "Render/DX11/Foundation/Core/pch_d3d11.h"
#include "Render/DX11/Foundation/Core/Device.hpp"
#include "Common/Debug/Logger.hpp"

using namespace Render::DX11::Foundation::Core;
using namespace Microsoft::WRL;

Device::Device() {}

Device::~Device() { CleanUp(); }

void Device::DumpRefCount(Common::Debug::LogFile* pLogFile, IUnknown* unk, const wchar_t* name) {
#ifdef _DEBUG
	if (!unk) { WLogln(pLogFile, std::format(L"{} = null", name)); return; }

	ULONG afterAdd = unk->AddRef();
	ULONG afterRel = unk->Release();

	WLogln(pLogFile, std::format(L"{} refcount ~= {}", name, afterRel));
#endif
}

BOOL Device::CreateSwapChainForHwnd(
	HWND hWnd,
	const DXGI_SWAP_CHAIN_DESC1* pDesc,
	const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc,
	IDXGIOutput* pRestrictToOutput,
	IDXGISwapChain1** ppSwapChain) {
	CheckHRESULT(mpLogFile, mFactory->CreateSwapChainForHwnd(
		mDevice.Get(), hWnd, pDesc, pFullscreenDesc, pRestrictToOutput, ppSwapChain));
	return TRUE;
}

BOOL Device::CreateRenderTargetView(
	ID3D11Resource* pResource,
	const D3D11_RENDER_TARGET_VIEW_DESC* pDesc,
	ID3D11RenderTargetView** ppRTView) {
	CheckHRESULT(mpLogFile, mDevice->CreateRenderTargetView(pResource, pDesc, ppRTView));
	return TRUE;
}

BOOL Device::CreateDepthStencilView(
	ID3D11Resource* pResource,
	const D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc,
	ID3D11DepthStencilView** ppDepthStencilView) {
	CheckHRESULT(mpLogFile, mDevice->CreateDepthStencilView(pResource, pDesc, ppDepthStencilView));
	return TRUE;
}

BOOL Device::CreateShaderResourceView(
	ID3D11Resource* pResource,
	const D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc,
	ID3D11ShaderResourceView** ppSRView) {
	CheckHRESULT(mpLogFile, mDevice->CreateShaderResourceView(pResource, pDesc, ppSRView));
	return TRUE;
}

BOOL Device::CreateUnorderedAccessView(
	ID3D11Resource* pResource,
	const D3D11_UNORDERED_ACCESS_VIEW_DESC* pDesc,
	ID3D11UnorderedAccessView** ppUAView) {
	CheckHRESULT(mpLogFile, mDevice->CreateUnorderedAccessView(pResource, pDesc, ppUAView));
	return TRUE;
}

BOOL Device::CreateTexture2D(
	const D3D11_TEXTURE2D_DESC* pDesc,
	const D3D11_SUBRESOURCE_DATA* pInitialData,
	ID3D11Texture2D** ppTexture2D) {
	CheckHRESULT(mpLogFile, mDevice->CreateTexture2D(pDesc, pInitialData, ppTexture2D));
	return TRUE;
}

BOOL Device::CreateBuffer(
	const D3D11_BUFFER_DESC* pDesc,
	const D3D11_SUBRESOURCE_DATA* pInitialData,
	ID3D11Buffer** ppBuffer) {
	CheckHRESULT(mpLogFile, mDevice->CreateBuffer(pDesc, pInitialData, ppBuffer));
	return TRUE;
}

BOOL Device::CreateInputLayout(
	const D3D11_INPUT_ELEMENT_DESC* pInputElementDescs,
	UINT NumElements,
	const void* pShaderBytecodeWithInputSignature,
	SIZE_T BytecodeLength,
	ID3D11InputLayout** ppInputLayout) {
	CheckHRESULT(mpLogFile, mDevice->CreateInputLayout(
		pInputElementDescs,
		NumElements,
		pShaderBytecodeWithInputSignature,
		BytecodeLength,
		ppInputLayout));
	return TRUE;
}

BOOL Device::CreateRasterizerState(
	const D3D11_RASTERIZER_DESC* pRasterizerDesc,
	ID3D11RasterizerState** ppRasterizerState) {
	CheckHRESULT(mpLogFile, mDevice->CreateRasterizerState(pRasterizerDesc, ppRasterizerState));
	return TRUE;
}

BOOL Device::CreateDepthStencilState(
	const D3D11_DEPTH_STENCIL_DESC* pDepthStencilDesc,
	ID3D11DepthStencilState** ppDepthStencilState) {
	CheckHRESULT(mpLogFile, mDevice->CreateDepthStencilState(pDepthStencilDesc, ppDepthStencilState));
	return TRUE;
}

BOOL Device::CreateBlendState(
	const D3D11_BLEND_DESC* pBlendStateDesc,
	ID3D11BlendState** ppBlendState) {
	CheckHRESULT(mpLogFile, mDevice->CreateBlendState(pBlendStateDesc, ppBlendState));
	return TRUE;
}

BOOL Device::CreateSamplerState(
	const D3D11_SAMPLER_DESC* pSamplerDesc,
	ID3D11SamplerState** ppSamplerState) {
	CheckHRESULT(mpLogFile, mDevice->CreateSamplerState(pSamplerDesc, ppSamplerState));
	return TRUE;
}

BOOL Device::CreateVertexShader(
		const void* pShaderBytecode,
		SIZE_T BytecodeLength,
		ID3D11ClassLinkage* pClassLinkage,
		ID3D11VertexShader** ppVertexShader) {
	CheckHRESULT(mpLogFile, mDevice->CreateVertexShader(
		pShaderBytecode, BytecodeLength, pClassLinkage, ppVertexShader));
	return TRUE;
}

BOOL Device::CreateGeometryShader(
		const void* pShaderBytecode,
		SIZE_T BytecodeLength,
		ID3D11ClassLinkage* pClassLinkage,
		ID3D11GeometryShader** ppGeometryShader) {
	return TRUE;
}

BOOL Device::CreatePixelShader(
		const void* pShaderBytecode,
		SIZE_T BytecodeLength,
		ID3D11ClassLinkage* pClassLinkage,
		ID3D11PixelShader** ppPixelShader) {
	return TRUE;
}

BOOL Device::CreateComputeShader(
		const void* pShaderBytecode,
		SIZE_T BytecodeLength,
		ID3D11ClassLinkage* pClassLinkage,
		ID3D11ComputeShader** ppComputeShader) {
	return TRUE;
}

BOOL Device::Initialize(Common::Debug::LogFile* const pLogFile) {
	mpLogFile = pLogFile;

	CheckReturn(mpLogFile, CreateDevice());
	CheckReturn(mpLogFile, CreateFactory());
#ifdef _DEBUG
	CheckReturn(mpLogFile, SetBreakOnSeverity());
#endif

	return TRUE;
}

void Device::CleanUp() {
	if (mbCleanedUp) return;

	if (mContext) {
		mContext->ClearState();
		mContext->Flush();
	}

	mContext.Reset();
	mFactory.Reset();
	mDevice.Reset();

	mbCleanedUp = TRUE;
}

BOOL Device::FlushDebugMessages() {
#ifdef _DEBUG
	CheckReturn(mpLogFile, FlushD3D11DebugMessages());
	CheckReturn(mpLogFile, FlushDXGIMessages());
#endif 

	return TRUE;
}

void Device::Flush() {
	mContext->ClearState();
	mContext->Flush();
}

BOOL Device::CreateDevice() {
	D3D_FEATURE_LEVEL level{};
#ifdef _DEBUG
	UINT flags = D3D11_CREATE_DEVICE_DEBUG;
#else
	UINT flags = 0;
#endif

	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	ComPtr<ID3D11DeviceContext> context{};
	CheckHRESULT(mpLogFile, D3D11CreateDevice(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		flags,
		featureLevels, _countof(featureLevels),
		D3D11_SDK_VERSION,
		&mDevice,
		&level,
		&context));

	CheckHRESULT(mpLogFile, context.As(&mContext));

	return TRUE;
}

BOOL Device::CreateFactory() {
	ComPtr<IDXGIDevice> dxgiDevice{};
	CheckHRESULT(mpLogFile, mDevice.As(&dxgiDevice));

	ComPtr<IDXGIAdapter> adapter{};
	CheckHRESULT(mpLogFile, dxgiDevice->GetAdapter(&adapter));

	CheckHRESULT(mpLogFile, adapter->GetParent(IID_PPV_ARGS(&mFactory)));

	return TRUE;
}

BOOL Device::SetBreakOnSeverity() {
	// IDXGIInfoQueue
	{
		ComPtr<IDXGIInfoQueue> dxgiInfoQueue{};
		CheckHRESULT(mpLogFile, DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue)));

		CheckHRESULT(mpLogFile, dxgiInfoQueue->SetBreakOnSeverity(
			DXGI_DEBUG_ALL,
			DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR,
			TRUE));
	}
	// ID3D11InfoQueue
	{
		ComPtr<ID3D11InfoQueue> infoQueue{};
		CheckHRESULT(mpLogFile, mDevice.As(&infoQueue));

		CheckHRESULT(mpLogFile, infoQueue->SetBreakOnSeverity(
			D3D11_MESSAGE_SEVERITY_ERROR, TRUE));
		CheckHRESULT(mpLogFile, infoQueue->SetBreakOnSeverity(
			D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE));
	}

	return TRUE;
}

BOOL Device::FlushDXGIMessages() {
	ComPtr<IDXGIInfoQueue> queue{};
	if (FAILED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&queue))) || !queue) return TRUE;

	const auto MessageCount = queue->GetNumStoredMessages(DXGI_DEBUG_ALL);

	for (UINT64 i = 0; i < MessageCount; ++i) {
		SIZE_T length = 0;
		queue->GetMessage(DXGI_DEBUG_ALL, i, nullptr, &length);

		std::vector<char> bytes(length);
		auto* message = reinterpret_cast<DXGI_INFO_QUEUE_MESSAGE*>(bytes.data());

		queue->GetMessage(DXGI_DEBUG_ALL, i, message, &length);

		Logln(mpLogFile, message->pDescription);
	}

	queue->ClearStoredMessages(DXGI_DEBUG_ALL);

	return TRUE;
}

BOOL Device::FlushD3D11DebugMessages() {
	if (!mDevice) return TRUE;

	ComPtr<ID3D11InfoQueue> queue{};
	if (FAILED(mDevice.As(&queue)) || !queue) return TRUE;

	const auto MessageCount = queue->GetNumStoredMessages();

	for (UINT64 i = 0; i < MessageCount; ++i) {
		SIZE_T messageLength{};
		queue->GetMessage(i, nullptr, &messageLength);

		std::vector<char> bytes(messageLength);
		decltype(auto) message = reinterpret_cast<D3D11_MESSAGE*>(bytes.data());

		queue->GetMessage(i, message, &messageLength);

		Logln(mpLogFile, message->pDescription);
	}

	queue->ClearStoredMessages();

	return TRUE;
}

void Device::ReportLiveObjects() {
	ComPtr<ID3D11Debug> debug{};
	if (SUCCEEDED(mDevice.As(&debug)))
		debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
}