#include "Render/DX11/Foundation/Core/pch_d3d11.h"
#include "Render/DX11/Shading/TAA.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX11/Foundation/Core/Device.hpp"
#include "Render/DX11/Foundation/Resource/FrameResource.hpp"
#include "Render/DX11/Shading/Util/ShaderManager.hpp"
#include "Render/DX11/Shading/Util/SamplerUtil.hpp"

using namespace Render::DX11::Shading;
using namespace DirectX;

namespace {
	const WCHAR* const HLSL_TAA = L"TAA.hlsl";
}

TAA::InitDataPtr TAA::MakeInitData() {
	return std::unique_ptr<TAAClass::InitData>(new TAAClass::InitData());
}

TAA::TAAClass::TAAClass() {}

TAA::TAAClass::~TAAClass() { CleanUp(); }

BOOL TAA::TAAClass::Initialize(
		Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	CheckReturn(mpLogFile, BuildResources());
	CheckReturn(mpLogFile, BuildDescriptors());

	for (size_t i = 0, end = mHaltonSequence.size(); i < end; ++i) {
		auto offset = mHaltonSequence[i];
		mFittedToBakcBufferHaltonSequence[i] = XMFLOAT2(
			((offset.x - 0.5f) / mInitData.Width) * 2.f,
			((offset.y - 0.5f) / mInitData.Height) * 2.f);
	}

	return TRUE;
}

void TAA::TAAClass::CleanUp() {
	if (mbCleanedUp) return;

	mRasterizerState.Reset();
	mDepthStencilState.Reset();
	mBlendState.Reset();

	mTaaVS.Reset();
	mTaaPS.Reset();

	mhHistoryMapSrv.Reset();
	mHistoryMap.Reset();

	mbCleanedUp = TRUE;
}

BOOL TAA::TAAClass::CompileShaders() {
	const auto VS = Util::ShaderManager::D3D11ShaderInfo{
		HLSL_TAA, nullptr, "VS", "vs_5_0", nullptr };
	CheckReturn(mpLogFile, mInitData.ShaderManager->CompileVertexShader(
		mInitData.Device, VS, mShaderHashes[Shader::VS_TAA],
		&mTaaVS));
	const auto PS = Util::ShaderManager::D3D11ShaderInfo{
		HLSL_TAA, nullptr, "PS", "ps_5_0", nullptr };
	CheckReturn(mpLogFile, mInitData.ShaderManager->CompilePixelShader(
		mInitData.Device, PS, mShaderHashes[Shader::PS_TAA],
		&mTaaPS));

	return TRUE;
}

BOOL TAA::TAAClass::BuildPipelineStates() {
	D3D11_RASTERIZER_DESC rs{};
	rs.FillMode = D3D11_FILL_SOLID;
	rs.CullMode = D3D11_CULL_BACK;
	rs.FrontCounterClockwise = FALSE;
	rs.DepthClipEnable = TRUE;
	CheckReturn(mpLogFile, mInitData.Device->CreateRasterizerState(&rs, &mRasterizerState));

	D3D11_DEPTH_STENCIL_DESC ds{};
	ds.DepthEnable = FALSE;
	ds.StencilEnable = FALSE;
	CheckReturn(mpLogFile, mInitData.Device->CreateDepthStencilState(&ds, &mDepthStencilState));

	D3D11_BLEND_DESC bd{};
	bd.AlphaToCoverageEnable = FALSE;
	bd.IndependentBlendEnable = FALSE;
	bd.RenderTarget[0].BlendEnable = FALSE;
	bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	CheckReturn(mpLogFile, mInitData.Device->CreateBlendState(&bd, &mBlendState));

	return TRUE;
}

BOOL TAA::TAAClass::OnResize(UINT width, UINT height) {
	mInitData.Width = width;
	mInitData.Height = height;

	CheckReturn(mpLogFile, BuildResources());
	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL TAA::TAAClass::Apply(
		Foundation::Resource::FrameResource* const pFrameResource,
		const D3D11_VIEWPORT& viewport,
		ID3D11Texture2D* const pBackBuffer,
		ID3D11RenderTargetView* const pBackBufferRtv,
		ID3D11Texture2D* const pBackBufferCopy,
		ID3D11ShaderResourceView* const pBackBufferCopySrv,
		ID3D11ShaderResourceView* const pVelocityMapSrv) {
	decltype(auto) context = mInitData.Device->Context();

	context->CopyResource(pBackBufferCopy, pBackBuffer);

	context->RSSetViewports(1, &viewport);

	context->RSSetState(mRasterizerState.Get());
	context->OMSetDepthStencilState(mDepthStencilState.Get(), 0);
	context->OMSetBlendState(mBlendState.Get(), nullptr, 0xFFFFFFFF);

	context->OMSetRenderTargets(1, &pBackBufferRtv, nullptr);

	ID3D11ShaderResourceView* srvs[] = { 
		pBackBufferCopySrv, mhHistoryMapSrv.Get(), pVelocityMapSrv };
	context->PSSetShaderResources(0, _countof(srvs), srvs);
	context->PSSetSamplers(
		0, Util::SamplerUtil::SamplerCount(), Util::SamplerUtil::GetSamplers());

	context->IASetInputLayout(nullptr);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	context->VSSetShader(mTaaVS.Get(), nullptr, 0);
	context->GSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(mTaaPS.Get(), nullptr, 0);

	context->Draw(6, 0);

	context->CopyResource(mHistoryMap.Get(), pBackBuffer);

	ID3D11ShaderResourceView* nullSrvs[3] = {};
	context->PSSetShaderResources(0, _countof(nullSrvs), nullSrvs);

	return TRUE;
}

BOOL TAA::TAAClass::BuildResources() {
	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = mInitData.Width;
	desc.Height = mInitData.Height;
	desc.Format = ShadingConvention::TAA::HistoryMapFormat;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	CheckReturn(mpLogFile, mInitData.Device->CreateTexture2D(&desc, nullptr, &mHistoryMap));

	return TRUE;
}

BOOL TAA::TAAClass::BuildDescriptors() {
	decltype(auto) map = mHistoryMap.Get();
	CheckReturn(mpLogFile, mInitData.Device->CreateShaderResourceView(
		map, nullptr, &mhHistoryMapSrv));

	return TRUE;
}