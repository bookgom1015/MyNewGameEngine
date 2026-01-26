#include "Render/DX11/Foundation/Core/pch_d3d11.h"
#include "Render/DX11/Shading/GammaCorrection.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX11/Foundation/Core/Device.hpp"
#include "Render/DX11/Foundation/Resource/FrameResource.hpp"
#include "Render/DX11/Shading/Util/ShaderManager.hpp"
#include "Render/DX11/Shading/Util/SamplerUtil.hpp"

using namespace Render::DX11::Shading;

namespace {
	const WCHAR* const HLSL_GammaCorrection = L"GammaCorrection.hlsl";
}

GammaCorrection::InitDataPtr GammaCorrection::MakeInitData() {
	return std::unique_ptr<GammaCorrectionClass::InitData>(new GammaCorrectionClass::InitData());
}

GammaCorrection::GammaCorrectionClass::GammaCorrectionClass() {}

GammaCorrection::GammaCorrectionClass::~GammaCorrectionClass() { CleanUp(); }

BOOL GammaCorrection::GammaCorrectionClass::Initialize(
		Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	return TRUE;
}

void GammaCorrection::GammaCorrectionClass::CleanUp() {
	if (mbCleanedUp) return;

	mRasterizerState.Reset();
	mDepthStencilState.Reset();
	mBlendState.Reset();

	mGammaCorrectionVS.Reset();
	mGammaCorrectionPS.Reset();

	mbCleanedUp = TRUE;
}

BOOL GammaCorrection::GammaCorrectionClass::CompileShaders() { 
	const auto VS = Util::ShaderManager::D3D11ShaderInfo{
		HLSL_GammaCorrection, nullptr, "VS", "vs_5_0", nullptr };
	CheckReturn(mpLogFile, mInitData.ShaderManager->CompileVertexShader(
		mInitData.Device, VS, mShaderHashes[Shader::VS_GammaCorrection],
		&mGammaCorrectionVS));
	const auto PS = Util::ShaderManager::D3D11ShaderInfo{
		HLSL_GammaCorrection, nullptr, "PS", "ps_5_0", nullptr };
	CheckReturn(mpLogFile, mInitData.ShaderManager->CompilePixelShader(
		mInitData.Device, PS, mShaderHashes[Shader::PS_GammaCorrection], 
		&mGammaCorrectionPS));

	return TRUE; 
}

BOOL GammaCorrection::GammaCorrectionClass::BuildPipelineStates() { 
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

BOOL GammaCorrection::GammaCorrectionClass::Apply(
		Foundation::Resource::FrameResource* const pFrameResource,
		const D3D11_VIEWPORT& viewport,
		ID3D11Texture2D* const pBackBuffer,
		ID3D11RenderTargetView* const pBackBufferRtv,
		ID3D11Texture2D* const pBackBufferCopy,
		ID3D11ShaderResourceView* const pBackBufferCopySrv) {
	decltype(auto) context = mInitData.Device->Context();

	context->CopyResource(pBackBufferCopy, pBackBuffer);

	context->RSSetViewports(1, &viewport);

	context->RSSetState(mRasterizerState.Get());
	context->OMSetDepthStencilState(mDepthStencilState.Get(), 0);
	context->OMSetBlendState(mBlendState.Get(), nullptr, 0xFFFFFFFF);

	context->OMSetRenderTargets(1, &pBackBufferRtv, nullptr);

	context->PSSetShaderResources(0, 1, &pBackBufferCopySrv);
	context->PSSetSamplers(
		0, Util::SamplerUtil::SamplerCount(), Util::SamplerUtil::GetSamplers());

	context->IASetInputLayout(nullptr);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);

	context->VSSetShader(mGammaCorrectionVS.Get(), nullptr, 0);
	context->GSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(mGammaCorrectionPS.Get(), nullptr, 0);

	context->Draw(6, 0);

	ID3D11ShaderResourceView* nullSrvs[1] = {};
	context->PSSetShaderResources(0, _countof(nullSrvs), nullSrvs);

	return TRUE;
}