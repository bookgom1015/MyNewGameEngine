#include "Render/DX/Foundation/Core/pch_d3d12.h"
#include "Render/DX/Foundation/Core/DepthStencilBuffer.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Foundation/Core/DescriptorHeap.hpp"
#include "Render/DX/Foundation/Resource/FrameResource.hpp"
#include "Render/DX/Foundation/Resource/GpuResource.hpp"
#include "Render/DX/Foundation/Util/D3D12Util.hpp"

using namespace Render::DX::Foundation::Core;

DepthStencilBuffer::DepthStencilBuffer() {
	mDepthStencilBuffer = std::make_unique<Resource::GpuResource>();
}

DepthStencilBuffer::~DepthStencilBuffer() { CleanUp(); }

UINT DepthStencilBuffer::CbvSrvUavDescCount() const { return 1; }

UINT DepthStencilBuffer::RtvDescCount() const { return 0; }

UINT DepthStencilBuffer::DsvDescCount() const { return 1; }

DepthStencilBuffer::InitDataPtr DepthStencilBuffer::MakeInitData() {
	return std::unique_ptr<InitData>(new InitData());
}

BOOL DepthStencilBuffer::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	CheckReturn(pLogFile, ShadingObject::Initialize(pLogFile, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	CheckReturn(mpLogFile, BuildDepthStencilBuffer());

	return TRUE;
}

void DepthStencilBuffer::CleanUp() {
	if (mDepthStencilBuffer) mDepthStencilBuffer.reset();
}

BOOL DepthStencilBuffer::BuildDescriptors(DescriptorHeap* const pDescHeap) {
	mhDepthStencilBufferCpuSrv = pDescHeap->CbvSrvUavCpuOffset(1);
	mhDepthStencilBufferGpuSrv = pDescHeap->CbvSrvUavGpuOffset(1);
	mhDepthStencilBufferCpuDsv = pDescHeap->DsvCpuOffset(0);

	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL DepthStencilBuffer::OnResize(UINT width, UINT height) {
	mInitData.ClientWidth = width;
	mInitData.ClientHeight = height;

	CheckReturn(mpLogFile, BuildDepthStencilBuffer());
	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL DepthStencilBuffer::BuildDepthStencilBuffer() {
	// Create the depth/stencil buffer and view.
	D3D12_RESOURCE_DESC depthStencilDesc{};
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = mInitData.ClientWidth;
	depthStencilDesc.Height = mInitData.ClientHeight;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = ShadingConvention::DepthStencilBuffer::DepthStencilBufferFormat;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear{};
	optClear.Format = ShadingConvention::DepthStencilBuffer::DepthStencilBufferFormat;
	optClear.DepthStencil.Depth = ShadingConvention::DepthStencilBuffer::InvalidDepthValue;
	optClear.DepthStencil.Stencil = ShadingConvention::DepthStencilBuffer::InvalidStencilValue;

	CheckReturn(mpLogFile, mDepthStencilBuffer->Initialize(
		mInitData.Device,
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_DEPTH_READ,
		&optClear,
		L"DepthStencilBuffer"
	));

	return TRUE;
}

BOOL DepthStencilBuffer::BuildDescriptors() {
	// Srv
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Format = ShadingConvention::DepthStencilBuffer::DepthBufferFormat;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.f;
		srvDesc.Texture2D.MipLevels = 1;

		Util::D3D12Util::CreateShaderResourceView(mInitData.Device, mDepthStencilBuffer->Resource(), &srvDesc, mhDepthStencilBufferCpuSrv);
	}
	// Dsv
	{
		// Create descriptor to mip level 0 of entire resource using the format of the resource.
		Util::D3D12Util::CreateDepthStencilView(mInitData.Device, mDepthStencilBuffer->Resource(), nullptr, mhDepthStencilBufferCpuDsv);
	}

	return TRUE;
}