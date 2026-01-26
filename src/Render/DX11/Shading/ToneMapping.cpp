#include "Render/DX11/Foundation/Core/pch_d3d11.h"
#include "Render/DX11/Shading/ToneMapping.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX11/Foundation/Core/Device.hpp"
#include "Render/DX11/Foundation/Resource/FrameResource.hpp"
#include "Render/DX11/Shading/Util/ShaderManager.hpp"
#include "Render/DX11/Shading/Util/SamplerUtil.hpp"

using namespace Render::DX11::Shading;

namespace {
	const WCHAR* const HLSL_ToneMapping = L"ToneMapping.hlsl";
}

ToneMapping::InitDataPtr ToneMapping::MakeInitData() {
	return std::unique_ptr<ToneMappingClass::InitData>(new ToneMappingClass::InitData());
}

ToneMapping::ToneMappingClass::ToneMappingClass() {}

ToneMapping::ToneMappingClass::~ToneMappingClass() { CleanUp(); }

BOOL ToneMapping::ToneMappingClass::Initialize(
		Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	CheckReturn(mpLogFile, BuildResources());
	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

void ToneMapping::ToneMappingClass::CleanUp() {
	if (mbCleanedUp) return;

	mRasterizerState.Reset();
	mDepthStencilState.Reset();
	mBlendState.Reset();

	mToneMappingVS.Reset();
	mToneMappingPS.Reset();

	mhIntermediateMapSrv.Reset();
	mhIntermediateMapRtv.Reset();
	mIntermediateMap.Reset();

	mbCleanedUp = TRUE;
}

BOOL ToneMapping::ToneMappingClass::CompileShaders() {
	const auto VS = Util::ShaderManager::D3D11ShaderInfo{
		HLSL_ToneMapping, nullptr, "VS", "vs_5_0", nullptr };
	CheckReturn(mpLogFile, mInitData.ShaderManager->CompileVertexShader(
		mInitData.Device, VS, mShaderHashes[Shader::VS_ToneMapping],
		&mToneMappingVS));
	const auto PS = Util::ShaderManager::D3D11ShaderInfo{
		HLSL_ToneMapping, nullptr, "PS", "ps_5_0", nullptr };
	CheckReturn(mpLogFile, mInitData.ShaderManager->CompilePixelShader(
		mInitData.Device, PS, mShaderHashes[Shader::PS_ToneMapping],
		&mToneMappingPS));

	return TRUE;
}

BOOL ToneMapping::ToneMappingClass::BuildPipelineStates() {
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

BOOL ToneMapping::ToneMappingClass::OnResize(UINT width, UINT height) {
	mInitData.Width = width;
	mInitData.Height = height;

	CheckReturn(mpLogFile, BuildResources());
	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL ToneMapping::ToneMappingClass::Apply(
		Foundation::Resource::FrameResource* const pFrameResource,
		const D3D11_VIEWPORT& viewport,
		ID3D11Texture2D* const pBackBuffer,
		ID3D11RenderTargetView* const pBackBufferRtv) {
	decltype(auto) context = mInitData.Device->Context();
	context->RSSetViewports(1, &viewport);

	context->RSSetState(mRasterizerState.Get());
	context->OMSetDepthStencilState(mDepthStencilState.Get(), 0);
	context->OMSetBlendState(mBlendState.Get(), nullptr, 0xFFFFFFFF);

	context->OMSetRenderTargets(1, &pBackBufferRtv, nullptr);

	ID3D11ShaderResourceView* srvs[] = { mhIntermediateMapSrv.Get() };
	context->PSSetShaderResources(0, 1, srvs);
	context->PSSetSamplers(
		0, Util::SamplerUtil::SamplerCount(), Util::SamplerUtil::GetSamplers());

	context->IASetInputLayout(nullptr);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	context->VSSetShader(mToneMappingVS.Get(), nullptr, 0);
	context->GSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(mToneMappingPS.Get(), nullptr, 0);

	context->Draw(6, 0);

	ID3D11ShaderResourceView* nullSrvs[1] = {};
	context->PSSetShaderResources(0, _countof(nullSrvs), nullSrvs);

	return TRUE;
}

BOOL ToneMapping::ToneMappingClass::BuildResources() {
	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = mInitData.Width;
	desc.Height = mInitData.Height;
	desc.Format = ShadingConvention::ToneMapping::IntermediateMapFormat;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	CheckReturn(mpLogFile, mInitData.Device->CreateTexture2D(&desc, nullptr, &mIntermediateMap));

	return TRUE;
}

BOOL ToneMapping::ToneMappingClass::BuildDescriptors() {
	decltype(auto) map = mIntermediateMap.Get();
	CheckReturn(mpLogFile, mInitData.Device->CreateRenderTargetView(
		map, nullptr, &mhIntermediateMapRtv));
	CheckReturn(mpLogFile, mInitData.Device->CreateShaderResourceView(
		map, nullptr, &mhIntermediateMapSrv));

	return TRUE;
}