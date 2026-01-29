#include "Render/DX11/Foundation/Core/pch_d3d11.h"
#include "Render/DX11/Shading/Shadow.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Mesh/Vertex.h"
#include "Render/DX11/Foundation/RenderItem.hpp"
#include "Render/DX11/Foundation/Core/Device.hpp"
#include "Render/DX11/Foundation/Resource/FrameResource.hpp"
#include "Render/DX11/Foundation/Resource/MeshGeometry.hpp"
#include "Render/DX11/Foundation/Resource/MaterialData.hpp"
#include "Render/DX11/Foundation/Util/D3D11Util.hpp"
#include "Render/DX11/Shading/Util/ShaderManager.hpp"
#include "Render/DX11/Shading/Util/SamplerUtil.hpp"

using namespace Render::DX11::Shading;

namespace {
	const WCHAR* const HLSL_DrawZDepth = L"DrawZDepth.hlsl";
	const WCHAR* const HLSL_DrawShadow = L"DrawShadow.hlsl";

	UINT ResolveShadowMapType(
		const Common::Foundation::Light* const light) {
		if (light->Type == Common::Foundation::LightType::E_Point
			|| light->Type == Common::Foundation::LightType::E_Tube)
			return ShadingConvention::Shadow::ShadowMapType_CubeMap;
		//else if (light->Type == Common::Foundation::LightType::E_Directional)
		//	return ShadingConvention::Shadow::ShadowMapType_Texture2DArray;
		else
			return ShadingConvention::Shadow::ShadowMapType_Texture2D;

	}
}

Shadow::InitDataPtr Shadow::MakeInitData() {
	return std::unique_ptr<ShadowClass::InitData>(new ShadowClass::InitData());
}

Shadow::ShadowClass::ShadowClass() {}

Shadow::ShadowClass::~ShadowClass() { CleanUp(); }

void Shadow::ShadowClass::Lights(std::vector<Common::Foundation::Light*>& lights) {
	for (UINT i = 0; i < mLightCount; ++i)
		lights.push_back(mLights[i].get());
}

BOOL Shadow::ShadowClass::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	mViewport = {
		.TopLeftX = 0,
		.TopLeftY = 0,
		.Width = static_cast<FLOAT>(mInitData.TextureWidth),
		.Height = static_cast<FLOAT>(mInitData.TextureHeight),
		.MinDepth = 0.f,
		.MaxDepth = 1.f
	};

	CheckReturn(mpLogFile, BuildResources());
	CheckReturn(mpLogFile, BuildDescriptors());


	return TRUE;
}

void Shadow::ShadowClass::CleanUp() {
	if (mbCleanedUp) return;

	mhShadowMapSrv.Reset();
	mhShadowMapUav.Reset();
	mShadowMap.Reset();

	for (UINT i = 0; i < MaxLights; ++i) {
		mhZDepthMapSrvs[i].Reset();
		mhZDepthMapDsvs[i].Reset();
		mZDepthMaps[i].Reset();
	}

	mDrawShadowCS.Reset();

	mInputLayout.Reset();
	mDrawZDepthVS.Reset();
	mDrawZDepthGS.Reset();
	mDrawZDepthPS.Reset();

	mRasterizerState.Reset();
	mDepthStencilState.Reset();
	mBlendState.Reset();

	mbCleanedUp = TRUE;
}

BOOL Shadow::ShadowClass::CompileShaders() {
	// DrawZDepth
	{
		const auto VS = Util::ShaderManager::D3D11ShaderInfo{
		HLSL_DrawZDepth, nullptr, "VS", "vs_5_0", nullptr };
		CheckReturn(mpLogFile, mInitData.ShaderManager->CompileVertexShader(
			mInitData.Device, VS, mShaderHashes[Shader::VS_DrawZDepth], &mDrawZDepthVS));

		const auto GS = Util::ShaderManager::D3D11ShaderInfo{
		HLSL_DrawZDepth, nullptr, "GS", "gs_5_0", nullptr };
		CheckReturn(mpLogFile, mInitData.ShaderManager->CompileGeometryShader(
			mInitData.Device, GS, mShaderHashes[Shader::GS_DrawZDepth], &mDrawZDepthGS));

		const auto PS = Util::ShaderManager::D3D11ShaderInfo{
			HLSL_DrawZDepth, nullptr, "PS", "ps_5_0", nullptr };
		CheckReturn(mpLogFile, mInitData.ShaderManager->CompilePixelShader(
			mInitData.Device, PS, mShaderHashes[Shader::PS_DrawZDepth], &mDrawZDepthPS));

		auto vsBlob = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::VS_DrawZDepth]);

		const D3D11_INPUT_ELEMENT_DESC* desc{};
		UINT count{};

		Foundation::Util::D3D11Util::GetDefaultInputLayout(desc, count);

		CheckReturn(mpLogFile, mInitData.Device->CreateInputLayout(
			desc, count,
			vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
			&mInputLayout));
	}
	// DrawShadow
	{
		const auto CS = Util::ShaderManager::D3D11ShaderInfo{
		HLSL_DrawShadow, nullptr, "CS", "cs_5_0", nullptr };
		CheckReturn(mpLogFile, mInitData.ShaderManager->CompileComputeShader(
			mInitData.Device, CS, mShaderHashes[Shader::CS_DrawShadow], &mDrawShadowCS));
	}

	return TRUE;
}

BOOL Shadow::ShadowClass::BuildPipelineStates() {
	D3D11_RASTERIZER_DESC rs{};
	rs.FillMode = D3D11_FILL_SOLID;
	rs.CullMode = D3D11_CULL_BACK;
	rs.FrontCounterClockwise = FALSE;
	rs.DepthClipEnable = TRUE;
	rs.DepthBias = 100000;
	rs.SlopeScaledDepthBias = 1.f;
	rs.DepthBiasClamp = 0.f;
	CheckReturn(mpLogFile, mInitData.Device->CreateRasterizerState(&rs, &mRasterizerState));

	D3D11_DEPTH_STENCIL_DESC ds{};
	ds.DepthEnable = TRUE;
	ds.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	ds.DepthFunc = D3D11_COMPARISON_LESS;
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

BOOL Shadow::ShadowClass::OnResize(UINT width, UINT height) {
	mInitData.ClientWidth = width;
	mInitData.ClientHeight = height;

	CheckReturn(mpLogFile, BuildResources());
	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL Shadow::ShadowClass::AddLight(const std::shared_ptr<Common::Foundation::Light>& light) {
	if (mLightCount >= MaxLights) ReturnFalse(mpLogFile, L"Can not add light due to the light count limit");

	const auto ShadowMapType = ResolveShadowMapType(light.get());

	CheckReturn(mpLogFile, BuildResource(ShadowMapType));
	CheckReturn(mpLogFile, BuildDescriptor(ShadowMapType));

	mLights[mLightCount++] = light;

	return TRUE; 
}

BOOL Shadow::ShadowClass::Run(
		Foundation::Resource::FrameResource* const pFrameResource,
		ID3D11ShaderResourceView* pPositionMapSrv,
		Foundation::RenderItem** ppRitems,
		UINT numRitems) {
	CheckReturn(mpLogFile, DrawZDepth(pFrameResource, ppRitems, numRitems));
	CheckReturn(mpLogFile, DrawShadow(pFrameResource, pPositionMapSrv));

	return TRUE;
}

BOOL Shadow::ShadowClass::BuildResources() {
	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = mInitData.ClientWidth;
	desc.Height = mInitData.ClientHeight;
	desc.Format = ShadingConvention::Shadow::ShadowMapFormat;
	desc.ArraySize = 1;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	CheckReturn(mpLogFile, mInitData.Device->CreateTexture2D(
		&desc, nullptr, &mShadowMap));

	return TRUE;
}

BOOL Shadow::ShadowClass::BuildDescriptors() {
	D3D11_SHADER_RESOURCE_VIEW_DESC sr{};
	sr.Format = ShadingConvention::Shadow::ShadowMapFormat;
	sr.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	sr.Texture2D.MipLevels = 1;
	sr.Texture2D.MostDetailedMip = 0;

	D3D11_UNORDERED_ACCESS_VIEW_DESC ua{};
	ua.Format = ShadingConvention::Shadow::ShadowMapFormat;
	ua.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	ua.Texture2D.MipSlice = 0;

	auto map = mShadowMap.Get();
	CheckReturn(mpLogFile, mInitData.Device->CreateShaderResourceView(map, &sr, &mhShadowMapSrv));
	CheckReturn(mpLogFile, mInitData.Device->CreateUnorderedAccessView(map, &ua, &mhShadowMapUav));

	return TRUE;
}

BOOL Shadow::ShadowClass::BuildResource(UINT shadowMapType) {
	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = mInitData.TextureWidth;
	desc.Height = mInitData.TextureHeight;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	if (shadowMapType == ShadingConvention::Shadow::ShadowMapType_CubeMap) {
		desc.ArraySize = 6;
		desc.Format = ShadingConvention::Shadow::ZDepthMapFormat;
		CheckReturn(mpLogFile, mInitData.Device->CreateTexture2D(
			&desc, nullptr, &mZDepthMaps[mLightCount]));
	}
	else {
		desc.ArraySize = 1;
		desc.Format = ShadingConvention::Shadow::ZDepthMapFormat;
		CheckReturn(mpLogFile, mInitData.Device->CreateTexture2D(
			&desc, nullptr, &mZDepthMaps[mLightCount]));
	}

	return TRUE;
}

BOOL Shadow::ShadowClass::BuildDescriptor(UINT shadowMapType) {
	decltype(auto) map = mZDepthMaps[mLightCount].Get();
	D3D11_DEPTH_STENCIL_VIEW_DESC ds{};
	ds.Format = DXGI_FORMAT_D32_FLOAT;

	D3D11_SHADER_RESOURCE_VIEW_DESC sr{};
	sr.Format = DXGI_FORMAT_R32_FLOAT;

	if (shadowMapType == ShadingConvention::Shadow::ShadowMapType_CubeMap) {
		ds.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
		ds.Texture2DArray.ArraySize = 6;
		ds.Texture2DArray.FirstArraySlice = 0;
		ds.Texture2DArray.MipSlice = 0;

		CheckReturn(mpLogFile, mInitData.Device->CreateDepthStencilView(
			map, &ds, &mhZDepthMapDsvs[mLightCount]));

		sr.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		sr.Texture2DArray.ArraySize = 6;
		sr.Texture2DArray.FirstArraySlice = 0;
		sr.Texture2DArray.MipLevels = 1;
		sr.Texture2DArray.MostDetailedMip = 0;

		CheckReturn(mpLogFile, mInitData.Device->CreateShaderResourceView(
			map, &sr, &mhZDepthMapSrvs[mLightCount]));
	}
	else {
		ds.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		ds.Texture2D.MipSlice = 0;

		CheckReturn(mpLogFile, mInitData.Device->CreateDepthStencilView(
			map, &ds, &mhZDepthMapDsvs[mLightCount]));

		sr.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		sr.Texture2D.MipLevels = 1;
		sr.Texture2D.MostDetailedMip = 0;

		CheckReturn(mpLogFile, mInitData.Device->CreateShaderResourceView(
			map, &sr, &mhZDepthMapSrvs[mLightCount]));
	}

	return TRUE;
}

BOOL Shadow::ShadowClass::DrawZDepth(
		Foundation::Resource::FrameResource* const pFrameResource,
		Foundation::RenderItem** ppRitems,
		UINT numRitems) {
	decltype(auto) context = mInitData.Device->Context();
	context->RSSetViewports(1, &mViewport);

	context->RSSetState(mRasterizerState.Get());
	context->OMSetDepthStencilState(mDepthStencilState.Get(), 0);
	context->OMSetBlendState(mBlendState.Get(), nullptr, 0xFFFFFFFF);

	context->VSSetShader(mDrawZDepthVS.Get(), nullptr, 0);
	context->GSSetShader(mDrawZDepthGS.Get(), nullptr, 0);
	context->PSSetShader(mDrawZDepthPS.Get(), nullptr, 0);
	
	context->IASetInputLayout(mInputLayout.Get());

	// LightCB
	{
		auto& lightCB = pFrameResource->LightCB;
		auto firstConstant = lightCB.FirstConstant(0);
		auto numConstants = lightCB.NumConstants();
		context->VSSetConstantBuffers1(
			0, 1, lightCB.CBAddress(), &firstConstant, &numConstants);
		context->GSSetConstantBuffers1(
			0, 1, lightCB.CBAddress(), &firstConstant, &numConstants);
		context->PSSetConstantBuffers1(
			0, 1, lightCB.CBAddress(), &firstConstant, &numConstants);
	}

	for (UINT lightIndex = 0; lightIndex < mLightCount; ++lightIndex) {
		auto dsv = mhZDepthMapDsvs[lightIndex].Get();
		context->ClearDepthStencilView(
			dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
		context->OMSetRenderTargets(0, nullptr, dsv);

		for (UINT i = 0; i < numRitems; ++i) {
			auto ritem = ppRitems[i];

			context->IASetPrimitiveTopology(ritem->PrimitiveType);

			// ShadowCB
			{
				auto& shadowCB = pFrameResource->ShadowCB;

				ShadowCB cb{};
				cb.LightIndex = lightIndex;

				CheckReturn(mpLogFile, shadowCB.SetData(cb));

				auto firstConstant = shadowCB.FirstConstant(0);
				auto numConstants = shadowCB.NumConstants();
				context->VSSetConstantBuffers1(
					3, 1, shadowCB.CBAddress(), &firstConstant, &numConstants);
				context->GSSetConstantBuffers1(
					3, 1, shadowCB.CBAddress(), &firstConstant, &numConstants);
				context->PSSetConstantBuffers1(
					3, 1, shadowCB.CBAddress(), &firstConstant, &numConstants);
			}
			// ObjectCB
			{
				auto& objCB = pFrameResource->ObjectCB;
				auto firstConstant = objCB.FirstConstant(ritem->ObjectCBIndex);
				auto numConstants = objCB.NumConstants();
				context->VSSetConstantBuffers1(
					1, 1, objCB.CBAddress(), &firstConstant, &numConstants);
				context->GSSetConstantBuffers1(
					1, 1, objCB.CBAddress(), &firstConstant, &numConstants);
				context->PSSetConstantBuffers1(
					1, 1, objCB.CBAddress(), &firstConstant, &numConstants);
			}
			// MaterialCB
			{
				auto& matCB = pFrameResource->MaterialCB;
				auto firstConstant = matCB.FirstConstant(ritem->Material->MaterialCBIndex);
				auto numConstants = matCB.NumConstants();
				context->VSSetConstantBuffers1(
					2, 1, matCB.CBAddress(), &firstConstant, &numConstants);
				context->GSSetConstantBuffers1(
					2, 1, matCB.CBAddress(), &firstConstant, &numConstants);
				context->PSSetConstantBuffers1(
					2, 1, matCB.CBAddress(), &firstConstant, &numConstants);
			}

			static const UINT Stride = sizeof(Common::Foundation::Mesh::Vertex);
			static const UINT Offset = 0;

			context->IASetVertexBuffers(
				0, 1, ritem->Geometry->VertexBufferAddress(), &Stride, &Offset);
			context->IASetIndexBuffer(
				ritem->Geometry->IndexBufferAddress(), ritem->Geometry->IndexFormat(), 0);

			context->DrawIndexed(ritem->IndexCount, 0, 0);
		}
	}

	context->OMSetRenderTargets(0, nullptr, nullptr);

	return TRUE;
}
BOOL Shadow::ShadowClass::DrawShadow(
		Foundation::Resource::FrameResource* const pFrameResource,
		ID3D11ShaderResourceView* pPositionMapSrv) {
	decltype(auto) context = mInitData.Device->Context();

	context->CSSetShader(mDrawShadowCS.Get(), nullptr, 0);

	context->CSSetShaderResources(0, 1, &pPositionMapSrv);
	context->CSSetSamplers(0, Util::SamplerUtil::SamplerCount(), Util::SamplerUtil::GetSamplers());

	ID3D11UnorderedAccessView* uavs[] = { mhShadowMapUav.Get() };
	UINT initialCounts[] = { 0 };
	context->CSSetUnorderedAccessViews(0, _countof(uavs), uavs, initialCounts);

	// LightCB
	{
		auto& lightCB = pFrameResource->LightCB;
		auto firstConstant = lightCB.FirstConstant(0);
		auto numConstants = lightCB.NumConstants();
		context->CSSetConstantBuffers1(
			0, 1, lightCB.CBAddress(), &firstConstant, &numConstants);
	}

	for (UINT lightIndex = 0; lightIndex < mLightCount; ++lightIndex) {
		ID3D11ShaderResourceView* zSrv = mhZDepthMapSrvs[lightIndex].Get();
		context->CSSetShaderResources(1, 1, &zSrv);

		// ShadowCB
		{
			auto& shadowCB = pFrameResource->ShadowCB;

			ShadowCB cb{};
			cb.LightIndex = lightIndex;

			CheckReturn(mpLogFile, shadowCB.SetData(cb));

			auto firstConstant = shadowCB.FirstConstant(0);
			auto numConstants = shadowCB.NumConstants();
			context->CSSetConstantBuffers1(
				1, 1, shadowCB.CBAddress(), &firstConstant, &numConstants);
		}

		context->Dispatch(
			Foundation::Util::D3D11Util::CeilDivide(mInitData.ClientWidth, 8),
			Foundation::Util::D3D11Util::CeilDivide(mInitData.ClientHeight, 8),
			1);
	}

	ID3D11ShaderResourceView* nullSrvs[2] = {};
	context->CSSetShaderResources(0, _countof(nullSrvs), nullSrvs);

	ID3D11UnorderedAccessView* nullUavs[1] = {};
	context->CSSetUnorderedAccessViews(0, _countof(nullUavs), nullUavs, nullptr);

	return TRUE;
}