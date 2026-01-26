#include "Render/DX11/Foundation/Core/pch_d3d11.h"
#include "Render/DX11/Shading/GBuffer.hpp"
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

namespace {
	const WCHAR* const HLSL_GBuffer = L"GBuffer.hlsl";
}

GBuffer::InitDataPtr GBuffer::MakeInitData() {
	return std::unique_ptr<GBufferClass::InitData>(new GBufferClass::InitData());
}

GBuffer::GBufferClass::GBufferClass() {}

GBuffer::GBufferClass::~GBufferClass() { CleanUp(); }

BOOL GBuffer::GBufferClass::Initialize(
		Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, Foundation::ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	CheckReturn(mpLogFile, BuildResources());
	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

void GBuffer::GBufferClass::CleanUp() {
	if (mbCleanedUp) return;

	mInputLayout.Reset();
	mGBufferVS.Reset();
	mGBufferPS.Reset();

	mRasterizerState.Reset();
	mDepthStencilState.Reset();
	mBlendState.Reset();

	for (auto& srv : mhSrvs) srv.Reset();
	for (auto& rtv : mhRtvs) rtv.Reset();
	for (auto& resource : mResources) resource.Reset();

	mbCleanedUp = TRUE;
}

BOOL GBuffer::GBufferClass::CompileShaders() {
	const auto VS = Util::ShaderManager::D3D11ShaderInfo{
		HLSL_GBuffer, nullptr, "VS", "vs_5_0", nullptr };
	CheckReturn(mpLogFile, mInitData.ShaderManager->CompileVertexShader(
		mInitData.Device, VS, mShaderHashes[Shader::VS_GBuffer], &mGBufferVS));
	const auto PS = Util::ShaderManager::D3D11ShaderInfo{
		HLSL_GBuffer, nullptr, "PS", "ps_5_0", nullptr };
	CheckReturn(mpLogFile, mInitData.ShaderManager->CompilePixelShader(
		mInitData.Device, PS, mShaderHashes[Shader::PS_GBuffer], &mGBufferPS));

	auto vsBlob = mInitData.ShaderManager->GetShader(mShaderHashes[Shader::VS_GBuffer]);

	const D3D11_INPUT_ELEMENT_DESC* desc{};
	UINT count{};

	Foundation::Util::D3D11Util::GetDefaultInputLayout(desc, count);

	CheckReturn(mpLogFile, mInitData.Device->CreateInputLayout(
		desc, count,
		vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
		&mInputLayout));

	return TRUE;
}

BOOL GBuffer::GBufferClass::BuildPipelineStates() {
	D3D11_RASTERIZER_DESC rs{};
	rs.FillMode = D3D11_FILL_SOLID;
	rs.CullMode = D3D11_CULL_BACK;
	rs.FrontCounterClockwise = FALSE;
	rs.DepthClipEnable = TRUE;
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

BOOL GBuffer::GBufferClass::OnResize(UINT width, UINT height) {
	mInitData.Width = width;
	mInitData.Height = height;

	CheckReturn(mpLogFile, BuildResources());
	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL GBuffer::GBufferClass::BuildResources() {
	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = mInitData.Width;
	desc.Height = mInitData.Height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	// AlbedoMap
	{
		desc.Format = ShadingConvention::GBuffer::AlbedoMapFormat;
		CheckReturn(mpLogFile, mInitData.Device->CreateTexture2D(
			&desc, nullptr, &mResources[Resource::E_Albedo]));
	}
	// NormalMap
	{
		desc.Format = ShadingConvention::GBuffer::NormalMapFormat;
		CheckReturn(mpLogFile, mInitData.Device->CreateTexture2D(
			&desc, nullptr, &mResources[Resource::E_Normal]));
	}
	// PositionMap
	{
		desc.Format = ShadingConvention::GBuffer::PositionMapFormat;
		CheckReturn(mpLogFile, mInitData.Device->CreateTexture2D(
			&desc, nullptr, &mResources[Resource::E_Position]));
	}
	// RoughnessMetalnessMap
	{
		desc.Format = ShadingConvention::GBuffer::RoughnessMetalnessMapFormat;
		CheckReturn(mpLogFile, mInitData.Device->CreateTexture2D(
			&desc, nullptr, &mResources[Resource::E_RoughnessMetalness]));
	}
	// VelocityMap
	{
		desc.Format = ShadingConvention::GBuffer::VelocityMapFormat;
		CheckReturn(mpLogFile, mInitData.Device->CreateTexture2D(
			&desc, nullptr, &mResources[Resource::E_Velocity]));
	}

	return TRUE;
}

BOOL GBuffer::GBufferClass::BuildDescriptors() {
	// AlbedoMap
	{
		decltype(auto) map = mResources[Resource::E_Albedo].Get();
		CheckReturn(mpLogFile, mInitData.Device->CreateRenderTargetView(
			map, nullptr, &mhRtvs[Descriptor::Rtv::E_Albedo]));
		CheckReturn(mpLogFile, mInitData.Device->CreateShaderResourceView(
			map, nullptr, &mhSrvs[Descriptor::Srv::E_Albedo]));
	}
	// NormalMap
	{
		decltype(auto) map = mResources[Resource::E_Normal].Get();
		CheckReturn(mpLogFile, mInitData.Device->CreateRenderTargetView(
			map, nullptr, &mhRtvs[Descriptor::Rtv::E_Normal]));
		CheckReturn(mpLogFile, mInitData.Device->CreateShaderResourceView(
			map, nullptr, &mhSrvs[Descriptor::Srv::E_Normal]));
	}
	// PositionMap
	{
		decltype(auto) map = mResources[Resource::E_Position].Get();
		CheckReturn(mpLogFile, mInitData.Device->CreateRenderTargetView(
			map, nullptr, &mhRtvs[Descriptor::Rtv::E_Position]));
		CheckReturn(mpLogFile, mInitData.Device->CreateShaderResourceView(
			map, nullptr, &mhSrvs[Descriptor::Srv::E_Position]));
	}
	// RoughnessMetalnessMap
	{
		decltype(auto) map = mResources[Resource::E_RoughnessMetalness].Get();
		CheckReturn(mpLogFile, mInitData.Device->CreateRenderTargetView(
			map, nullptr, &mhRtvs[Descriptor::Rtv::E_RoughnessMetalness]));
		CheckReturn(mpLogFile, mInitData.Device->CreateShaderResourceView(
			map, nullptr, &mhSrvs[Descriptor::Srv::E_RoughnessMetalness]));
	}
	// Velocity
	{
		decltype(auto) map = mResources[Resource::E_Velocity].Get();
		CheckReturn(mpLogFile, mInitData.Device->CreateRenderTargetView(
			map, nullptr, &mhRtvs[Descriptor::Rtv::E_Velocity]));
		CheckReturn(mpLogFile, mInitData.Device->CreateShaderResourceView(
			map, nullptr, &mhSrvs[Descriptor::Srv::E_Velocity]));
	}


	return TRUE;
}

BOOL GBuffer::GBufferClass::DrawGBuffer(
		Foundation::Resource::FrameResource* const pFrameResource,
		const D3D11_VIEWPORT& viewport,
		ID3D11DepthStencilView* const pDsv,
		Foundation::RenderItem** ppRitems,
		UINT numRitems) {
	decltype(auto) context = mInitData.Device->Context();
	context->RSSetViewports(1, &viewport);

	context->ClearRenderTargetView(
		mhRtvs[Descriptor::Rtv::E_Albedo].Get(), 
		ShadingConvention::GBuffer::AlbedoMapClearValues);
	context->ClearRenderTargetView(
		mhRtvs[Descriptor::Rtv::E_Normal].Get(), 
		ShadingConvention::GBuffer::NormalMapClearValues);
	context->ClearRenderTargetView(
		mhRtvs[Descriptor::Rtv::E_Position].Get(), 
		ShadingConvention::GBuffer::PositionMapClearValues);
	context->ClearRenderTargetView(
		mhRtvs[Descriptor::Rtv::E_RoughnessMetalness].Get(), 
		ShadingConvention::GBuffer::RoughnessMapClearValues);
	context->ClearRenderTargetView(
		mhRtvs[Descriptor::Rtv::E_Velocity].Get(),
		ShadingConvention::GBuffer::VelocityMapClearValues);

	context->ClearDepthStencilView(
		pDsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

	ID3D11RenderTargetView* rtvs[] = {
		mhRtvs[Descriptor::Rtv::E_Albedo].Get(),
		mhRtvs[Descriptor::Rtv::E_Normal].Get(),
		mhRtvs[Descriptor::Rtv::E_Position].Get(),
		mhRtvs[Descriptor::Rtv::E_RoughnessMetalness].Get(),
		mhRtvs[Descriptor::Rtv::E_Velocity].Get()
	};
	context->OMSetRenderTargets(_countof(rtvs), rtvs, pDsv);

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
	// GBufferCB
	{
		auto& gbufferCB = pFrameResource->GBufferCB;
		auto firstConstant = gbufferCB.FirstConstant(0);
		auto numConstants = gbufferCB.NumConstants();
		context->VSSetConstantBuffers1(
			3, 1, gbufferCB.CBAddress(), &firstConstant, &numConstants);
		context->PSSetConstantBuffers1(
			3, 1, gbufferCB.CBAddress(), &firstConstant, &numConstants);
	}

	context->IASetInputLayout(mInputLayout.Get());
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->VSSetShader(mGBufferVS.Get(), nullptr, 0);
	context->GSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(mGBufferPS.Get(), nullptr, 0);

	for (UINT i = 0; i < numRitems; ++i) {
		auto ritem = ppRitems[i];

		// ObjectCB
		{
			auto& objCB = pFrameResource->ObjectCB;
			auto firstConstant = objCB.FirstConstant(ritem->ObjectCBIndex);
			auto numConstants = objCB.NumConstants();
			context->VSSetConstantBuffers1(
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
			context->PSSetConstantBuffers1(
				2, 1, matCB.CBAddress(), &firstConstant, &numConstants);
		}

		static const UINT Stride = sizeof(Common::Foundation::Mesh::Vertex);
		static const UINT Offset = 0;
		
		context->IASetVertexBuffers(
			0, 1, ritem->Geometry->VertexBufferAddress(), &Stride, &Offset);
		context->IASetIndexBuffer(
			ritem->Geometry->IndexBufferAddress(), DXGI_FORMAT_R32_UINT, 0);

		context->DrawIndexed(ritem->IndexCount, 0, 0);
	}

	return TRUE;
}