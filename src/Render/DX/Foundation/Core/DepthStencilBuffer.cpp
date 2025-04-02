#include "Render/DX/Foundation/Core/DepthStencilBuffer.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Foundation/Resource/GpuResource.hpp"
#include "Render/DX/Foundation/Util/D3D12Util.hpp"

using namespace Render::DX::Foundation::Core;

DepthStencilBuffer::DepthStencilBuffer() {
	mInitData = {};
	mDepthStencilBuffer = std::make_unique<Resource::GpuResource>();
	mhDepthStencilBufferCpuDsv = {};
}

DepthStencilBuffer::~DepthStencilBuffer() {}

UINT DepthStencilBuffer::CbvSrvUavDescCount() const { return 0; }
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

BOOL DepthStencilBuffer::BuildDescriptors(DescriptorHeap* const pDescHeap) {
	mhDepthStencilBufferCpuDsv = pDescHeap->DsvCpuOffset(0);

	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL DepthStencilBuffer::OnResize(UINT width, UINT height) {
	mInitData.Width = width;
	mInitData.Height = height;

	CheckReturn(mpLogFile, BuildDepthStencilBuffer());
	CheckReturn(mpLogFile, BuildDescriptors());

	return TRUE;
}

BOOL DepthStencilBuffer::BuildDepthStencilBuffer() {
	// Create the depth/stencil buffer and view.
	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = mInitData.Width;
	depthStencilDesc.Height = mInitData.Height;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = ShadingConvention::DepthStencilBuffer::DepthStencilBufferFormat;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = ShadingConvention::DepthStencilBuffer::DepthStencilBufferFormat;
	optClear.DepthStencil.Depth = 1.f;
	optClear.DepthStencil.Stencil = 0;

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
	// Create descriptor to mip level 0 of entire resource using the format of the resource.
	Util::D3D12Util::CreateDepthStencilView(mInitData.Device, mDepthStencilBuffer->Resource(), nullptr, mhDepthStencilBufferCpuDsv);

	return TRUE;
}