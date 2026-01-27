#include "Render/DX11/Foundation/Core/pch_d3d11.h"
#include "Render/DX11/Shading/BRDF.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX11/Foundation/Core/Device.hpp"
#include "Render/DX11/Foundation/Resource/FrameResource.hpp"
#include "Render/DX11/Shading/Util/ShaderManager.hpp"
#include "Render/DX11/Shading/Util/SamplerUtil.hpp"

using namespace Render::DX11::Shading;

namespace {
	const WCHAR* const HLSL_BRDF = L"ComputeBRDF.hlsl";
}

BRDF::InitDataPtr BRDF::MakeInitData() {
	return std::unique_ptr<BRDFClass::InitData>(new BRDFClass::InitData());
}

BRDF::BRDFClass::BRDFClass() {}

BRDF::BRDFClass::~BRDFClass() { CleanUp(); }

BOOL BRDF::BRDFClass::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	return TRUE;
}

void BRDF::BRDFClass::CleanUp() {
	if (mbCleanedUp) return;

	mBRDFVS.Reset();
	mBRDFPS.Reset();

	mRasterizerState.Reset();
	mDepthStencilState.Reset();
	mBlendState.Reset();

	mbCleanedUp = TRUE;
}

BOOL BRDF::BRDFClass::CompileShaders() {
	const auto VS = Util::ShaderManager::D3D11ShaderInfo{
		HLSL_BRDF, nullptr, "VS", "vs_5_0", nullptr };
	CheckReturn(mpLogFile, mInitData.ShaderManager->CompileVertexShader(
		mInitData.Device, VS, mShaderHashes[Shader::VS_ComputeBRDF], &mBRDFVS));
	const auto PS = Util::ShaderManager::D3D11ShaderInfo{
		HLSL_BRDF, nullptr, "PS", "ps_5_0", nullptr };
	CheckReturn(mpLogFile, mInitData.ShaderManager->CompilePixelShader(
		mInitData.Device, PS, mShaderHashes[Shader::PS_ComputeBRDF], &mBRDFPS));

	return TRUE;
}

BOOL BRDF::BRDFClass::BuildPipelineStates() {
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

BOOL BRDF::BRDFClass::ComputeBRDF(
		Foundation::Resource::FrameResource* const pFrameResource,
		const D3D11_VIEWPORT& viewport,
		ID3D11RenderTargetView* pBackBufferRtv,
		ID3D11ShaderResourceView* pDiffuseMapSrv,
		ID3D11ShaderResourceView* pNormalMapSrv,
		ID3D11ShaderResourceView* pPositionMapSrv,
		ID3D11ShaderResourceView* pRoughnessMetalnessMapSrv,
		ID3D11ShaderResourceView* pShadowMapSrv,
		UINT numLights,
		ID3D11ShaderResourceView* pDiffuseIrradianceCubeMapSrv,
		ID3D11ShaderResourceView* pPrefilteredEnvCubeMapSrv,
		ID3D11ShaderResourceView* pBrdfLutMapSrv) {
	decltype(auto) context = mInitData.Device->Context();
	context->RSSetViewports(1, &viewport);

	context->OMSetRenderTargets(1, &pBackBufferRtv, nullptr);

	context->RSSetState(mRasterizerState.Get());
	context->OMSetDepthStencilState(mDepthStencilState.Get(), 0);
	context->OMSetBlendState(mBlendState.Get(), nullptr, 0xFFFFFFFF);

	context->PSSetShaderResources(0, 1, &pDiffuseMapSrv);
	context->PSSetShaderResources(1, 1, &pNormalMapSrv);
	context->PSSetShaderResources(2, 1, &pPositionMapSrv);
	context->PSSetShaderResources(3, 1, &pRoughnessMetalnessMapSrv);
	context->PSSetShaderResources(4, 1, &pShadowMapSrv);
	context->PSSetShaderResources(5, 1, &pDiffuseIrradianceCubeMapSrv);
	context->PSSetShaderResources(6, 1, &pPrefilteredEnvCubeMapSrv);
	context->PSSetShaderResources(7, 1, &pBrdfLutMapSrv);
	context->PSSetSamplers(
		0, Util::SamplerUtil::SamplerCount(), Util::SamplerUtil::GetSamplers());

	context->IASetInputLayout(nullptr);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);

	context->VSSetShader(mBRDFVS.Get(), nullptr, 0);
	context->GSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(mBRDFPS.Get(), nullptr, 0);

	// PassCB
	{
		auto& passCB = pFrameResource->PassCB;
		auto firstConstant = passCB.FirstConstant(0);
		auto numConstants = passCB.NumConstants();
		context->VSSetConstantBuffers1(
			0, 1, passCB.CBAddress(), &firstConstant, &numConstants);
		context->PSSetConstantBuffers1(
			0, 1, passCB.CBAddress(), &firstConstant, &numConstants);
	}

	for (UINT i = 0; i < numLights; ++i) {
		// LightCB
		{
			auto& lightCB = pFrameResource->LightCB;
			auto firstConstant = lightCB.FirstConstant(i);
			auto numConstants = lightCB.NumConstants();
			context->VSSetConstantBuffers1(
				1, 1, lightCB.CBAddress(), &firstConstant, &numConstants);
			context->PSSetConstantBuffers1(
				1, 1, lightCB.CBAddress(), &firstConstant, &numConstants);
		}

		context->Draw(6, 0);
	}

	ID3D11ShaderResourceView* nullSrvs[8] = {};
	context->PSSetShaderResources(0, _countof(nullSrvs), nullSrvs);

	return TRUE;
}