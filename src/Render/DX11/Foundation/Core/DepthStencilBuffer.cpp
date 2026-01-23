#include "Render/DX11/Foundation/Core/pch_d3d11.h"
#include "Render/DX11/Foundation/Core/DepthStencilBuffer.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX11/Foundation/Core/Device.hpp"

using namespace Render::DX11::Foundation::Core;

std::unique_ptr<DepthStencilBuffer::InitData> DepthStencilBuffer::MakeInitData() {
	return std::unique_ptr<InitData>(new InitData());
}

DepthStencilBuffer::DepthStencilBuffer() {}

DepthStencilBuffer::~DepthStencilBuffer() {}

BOOL DepthStencilBuffer::Initialize(
		Common::Debug::LogFile* const pLogFile, void* const pData) {
	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	CheckReturn(mpLogFile, CreateDepthStencilBuffer());
	CheckReturn(mpLogFile, CreateDepthStencilBufferView());

	return TRUE;
}

void DepthStencilBuffer::CleanUp() {
	if (mbCleanedUp) return;

	mhDepthMapSrv.Reset();
	mhDepthStencilBufferView.Reset();
	mDepthStencilBuffer.Reset();

	mbCleanedUp = TRUE;
}

BOOL DepthStencilBuffer::OnResize(UINT width, UINT height) {
	mInitData.Width = width;
	mInitData.Height = height;

	CheckReturn(mpLogFile, CreateDepthStencilBuffer());
	CheckReturn(mpLogFile, CreateDepthStencilBufferView());

	return TRUE;
}

BOOL DepthStencilBuffer::CreateDepthStencilBuffer() {
	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = mInitData.Width;
	desc.Height = mInitData.Height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = ShadingConvention::DepthStencilBuffer::DepthStencilBufferFormat;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	CheckReturn(mpLogFile, mInitData.Device->CreateTexture2D(&desc, nullptr, &mDepthStencilBuffer));

	return TRUE;
}

BOOL DepthStencilBuffer::CreateDepthStencilBufferView() {
	D3D11_DEPTH_STENCIL_VIEW_DESC ds{};
	ds.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	ds.Format = ShadingConvention::DepthStencilBuffer::DepthStencilViewFormat;
	ds.Texture2D.MipSlice = 0;

	auto map = mDepthStencilBuffer.Get();
	CheckReturn(mpLogFile, mInitData.Device->CreateDepthStencilView(
		map, &ds, &mhDepthStencilBufferView));

	D3D11_SHADER_RESOURCE_VIEW_DESC sr{};
	sr.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	sr.Format = ShadingConvention::DepthStencilBuffer::ShaderResourceViewFormat;
	sr.Texture2D.MipLevels = 1;
	sr.Texture2D.MostDetailedMip = 0;

	CheckReturn(mpLogFile, mInitData.Device->CreateShaderResourceView(
		map, &sr, &mhDepthMapSrv));

	return TRUE;
}
