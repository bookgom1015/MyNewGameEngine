#include "Render/DX11/Foundation/Core/pch_d3d11.h"
#include "Render/DX11/Shading/GBuffer.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX11/Foundation/Core/Device.hpp"
#include "Render/DX11/Shading/Util/ShaderManager.hpp"

using namespace Render::DX11::Shading;

namespace {
	const WCHAR* const HLSL_GBuffer = L"GBuffer.hlsl";
}

GBuffer::InitDataPtr GBuffer::MakeInitData() {
	return std::unique_ptr<GBufferClass::InitData>(new GBufferClass::InitData());
}

GBuffer::GBufferClass::GBufferClass() {}

GBuffer::GBufferClass::~GBufferClass() {}

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
	for (auto& srv : mhSrvs) srv.Reset();
	for (auto& rtv : mhRtvs) rtv.Reset();
	for (auto& resource : mResources) resource.Reset();
}

BOOL GBuffer::GBufferClass::CompileShaders() {
	//const auto VS = Util::ShaderManager::D3D11ShaderInfo(HLSL_GBuffer, L"VS", L"vs_5_0");
	//const auto PS = Util::ShaderManager::D3D11ShaderInfo(HLSL_GBuffer, L"PS", L"ps_5_0");
	//CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(VS, mShaderHashes[Shader::VS_GBuffer]));
	//CheckReturn(mpLogFile, mInitData.ShaderManager->AddShader(PS, mShaderHashes[Shader::PS_GBuffer]));

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

	return TRUE;
}