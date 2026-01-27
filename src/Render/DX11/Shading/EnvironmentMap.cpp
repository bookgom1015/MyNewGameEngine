#include "Render/DX11/Foundation/Core/pch_d3d11.h"
#include "Render/DX11/Shading/EnvironmentMap.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Mesh/Vertex.h"
#include "Render/DX11/Foundation/RenderItem.hpp"
#include "Render/DX11/Foundation/Core/Device.hpp"
#include "Render/DX11/Foundation/Resource/FrameResource.hpp"
#include "Render/DX11/Foundation/Resource/MeshGeometry.hpp"
#include "Render/DX11/Foundation/Resource/MaterialData.hpp"
#include "Render/DX11/Foundation/Util/D3D11Util.hpp"
#include "Render/DX11/Shading/Util/ShaderManager.hpp"

using namespace Render::DX11::Shading;
using namespace DirectX;

namespace {
	const WCHAR* const HLSL_DrawSkySphere = L"DrawSkySphere.hlsl";
}

EnvironmentMap::InitDataPtr EnvironmentMap::MakeInitData() {
	return std::unique_ptr<EnvironmentMapClass::InitData>(new EnvironmentMapClass::InitData());
}

EnvironmentMap::EnvironmentMapClass::EnvironmentMapClass() {}

EnvironmentMap::EnvironmentMapClass::~EnvironmentMapClass() { CleanUp(); }

BOOL EnvironmentMap::EnvironmentMapClass::Initialize(
		Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	CheckReturn(mpLogFile, BuildResources());
	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

void EnvironmentMap::EnvironmentMapClass::CleanUp() {
	if (mbCleanedUp) return;

	mEnvironmentCubeMap.Reset();
	mDiffuseIrradianceCubeMap.Reset();
	mPrefilteredEnvironmentCubeMap.Reset();
	mBrdfLutMap.Reset();

	mInputLayout.Reset();
	mDrawSkySphereVS.Reset();
	mDrawSkySpherePS.Reset();

	mRasterizerState.Reset();
	mDepthStencilState.Reset();
	mBlendState.Reset();

	mbCleanedUp = TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::CompileShaders() {
	const auto VS = Util::ShaderManager::D3D11ShaderInfo{
		HLSL_DrawSkySphere , nullptr, "VS", "vs_5_0", nullptr };
	CheckReturn(mpLogFile, mInitData.ShaderManager->CompileVertexShader(
		mInitData.Device, VS, mShaderHashes[Shader::VS_DrawSkySphere],
		&mDrawSkySphereVS));
	const auto PS = Util::ShaderManager::D3D11ShaderInfo{
		HLSL_DrawSkySphere , nullptr, "PS", "ps_5_0", nullptr };
	CheckReturn(mpLogFile, mInitData.ShaderManager->CompilePixelShader(
		mInitData.Device, PS, mShaderHashes[Shader::PS_DrawSkySphere],
		&mDrawSkySpherePS));

	auto vsBlob = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::VS_DrawSkySphere]);

	const D3D11_INPUT_ELEMENT_DESC* desc{};
	UINT count{};

	Foundation::Util::D3D11Util::GetDefaultInputLayout(desc, count);

	CheckReturn(mpLogFile, mInitData.Device->CreateInputLayout(
		desc, count,
		vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
		&mInputLayout));

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::BuildPipelineStates() {
	D3D11_RASTERIZER_DESC rs{};
	rs.FillMode = D3D11_FILL_SOLID;
	rs.CullMode = D3D11_CULL_FRONT;
	rs.FrontCounterClockwise = FALSE;
	rs.DepthClipEnable = TRUE;
	CheckReturn(mpLogFile, mInitData.Device->CreateRasterizerState(&rs, &mRasterizerState));

	D3D11_DEPTH_STENCIL_DESC ds{};
	ds.DepthEnable = TRUE;
	ds.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	ds.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
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

BOOL EnvironmentMap::EnvironmentMapClass::DrawSkySphere(
		Foundation::Resource::FrameResource* const pFrameResource,
		const D3D11_VIEWPORT& viewport,
		ID3D11RenderTargetView* const pBackBufferRtv,
		ID3D11DepthStencilView* const pDsv,
		Foundation::RenderItem* pSphere) {
	decltype(auto) context = mInitData.Device->Context();
	context->RSSetViewports(1, &viewport);

	context->OMSetRenderTargets(1, &pBackBufferRtv, pDsv);

	context->RSSetState(mRasterizerState.Get());
	context->OMSetDepthStencilState(mDepthStencilState.Get(), 0);
	context->OMSetBlendState(mBlendState.Get(), nullptr, 0xFFFFFFFF);

	// MainPassCB
	{
		auto& passCB = pFrameResource->PassCB;
		auto firstConstant = passCB.FirstConstant(0);
		auto numConstants = passCB.NumConstants();
		context->VSSetConstantBuffers1(
			0, 1, passCB.CBAddress(), &firstConstant, &numConstants);
		context->PSSetConstantBuffers1(
			0, 1, passCB.CBAddress(), &firstConstant, &numConstants);
	}

	ID3D11ShaderResourceView* srvs[] = { mhEnvironmentCubeMapSrv.Get() };
	context->PSSetShaderResources(0, _countof(srvs), srvs);

	context->IASetInputLayout(mInputLayout.Get());
	context->IASetPrimitiveTopology(pSphere->PrimitiveType);

	context->VSSetShader(mDrawSkySphereVS.Get(), nullptr, 0);
	context->GSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(mDrawSkySpherePS.Get(), nullptr, 0);

	static const UINT Stride = sizeof(Common::Foundation::Mesh::Vertex);
	static const UINT Offset = 0;

	context->IASetVertexBuffers(
		0, 1, pSphere->Geometry->VertexBufferAddress(), &Stride, &Offset);
	context->IASetIndexBuffer(
		pSphere->Geometry->IndexBufferAddress(), pSphere->Geometry->IndexFormat(), 0);

	context->DrawIndexed(pSphere->IndexCount, 0, 0);

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::BuildResources() {
	CheckReturn(mpLogFile, Foundation::Util::D3D11Util::LoadDDStoTexture(
		L"./../../../assets/Textures/forest_hdr_env_cube_map.dds",
		mInitData.Device,
		mEnvironmentCubeMap));

	CheckReturn(mpLogFile, Foundation::Util::D3D11Util::LoadDDStoTexture(
		L"./../../../assets/Textures/forest_hdr_diff_irrad_cube_map.dds",
		mInitData.Device,
		mDiffuseIrradianceCubeMap));

	CheckReturn(mpLogFile, Foundation::Util::D3D11Util::LoadDDStoTexture(
		L"./../../../assets/Textures/forest_hdr_prefiltered_env_cube_map.dds",
		mInitData.Device,
		mPrefilteredEnvironmentCubeMap));

	CheckReturn(mpLogFile, Foundation::Util::D3D11Util::LoadDDStoTexture(
		L"./../../../assets/Textures/brdf_lut_map.dds",
		mInitData.Device,
		mBrdfLutMap));

	return TRUE;
}

BOOL EnvironmentMap::EnvironmentMapClass::BuildDescriptors() {
	CheckReturn(mpLogFile, mInitData.Device->CreateShaderResourceView(
		mEnvironmentCubeMap.Get(), nullptr, &mhEnvironmentCubeMapSrv));

	CheckReturn(mpLogFile, mInitData.Device->CreateShaderResourceView(
		mDiffuseIrradianceCubeMap.Get(), nullptr, &mhDiffuseIrradianceCubeMapSrv));

	CheckReturn(mpLogFile, mInitData.Device->CreateShaderResourceView(
		mPrefilteredEnvironmentCubeMap.Get(), nullptr, &mhPrefilteredEnvironmentCubeMapSrv));

	CheckReturn(mpLogFile, mInitData.Device->CreateShaderResourceView(
		mBrdfLutMap.Get(), nullptr, &mhBrdfLutMapSrv));

	return TRUE;
}