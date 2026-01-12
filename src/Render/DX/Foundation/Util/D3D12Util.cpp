#include "Render/DX/Foundation/Util/D3D12Util.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX/Foundation/Core/Factory.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Foundation/Core/CommandObject.hpp"
#include "Render/DX/Foundation/Resource/GpuResource.hpp"
#include "Render/DX/Foundation/Resource/Texture.hpp"

#include <vector>

#include <DDSTextureLoader.h>
#include <ResourceUploadBatch.h>

namespace {
	const D3D12_INPUT_ELEMENT_DESC gInputLayout[] = {
		{ "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT,	0, 0,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",		0, DXGI_FORMAT_R32G32B32_FLOAT,	0, 12,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",	0, DXGI_FORMAT_R32G32_FLOAT,	0, 24,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	const D3D12_INPUT_LAYOUT_DESC gInputLayoutDesc = { gInputLayout, static_cast<UINT>(_countof(gInputLayout)) };
}

using namespace Render::DX::Foundation::Util;
using namespace Microsoft::WRL;
using namespace DirectX;

Common::Debug::LogFile* D3D12Util::mpLogFile = nullptr;

D3D12Util::D3D12BufferCreateInfo::D3D12BufferCreateInfo() {}

D3D12Util::D3D12BufferCreateInfo::D3D12BufferCreateInfo(UINT64 size, D3D12_RESOURCE_FLAGS flags) : Size(size), Flags(flags) {}

D3D12Util::D3D12BufferCreateInfo::D3D12BufferCreateInfo(UINT64 size, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES state) : Size(size), HeapType(heapType), State(state) {}

D3D12Util::D3D12BufferCreateInfo::D3D12BufferCreateInfo(UINT64 size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state) : Size(size), Flags(flags), State(state) {}

D3D12Util::D3D12BufferCreateInfo::D3D12BufferCreateInfo(UINT64 size, UINT64 alignment, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state)
	: Size(size), Alignment(alignment), HeapType(heapType), Flags(flags), State(state) {}

D3D12Util::D3D12BufferCreateInfo::D3D12BufferCreateInfo(UINT64 size, UINT64 alignment, D3D12_HEAP_TYPE heapType, D3D12_HEAP_FLAGS heapFlags, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state)
	: Size(size), Alignment(alignment), HeapType(heapType), HeapFlags(heapFlags), Flags(flags), State(state) {}

BOOL D3D12Util::Initialize(Common::Debug::LogFile* const pLogFile) {
	mpLogFile = pLogFile;

	return TRUE;
}

BOOL D3D12Util::CreateSwapChain(
		Core::Factory* const pFactory,
		Core::CommandObject* const pCmdObject,
		DXGI_SWAP_CHAIN_DESC* pDesc,
		IDXGISwapChain** ppSwapChain) {
	CheckHRESULT(mpLogFile, pFactory->mDxgiFactory->CreateSwapChain(
		pCmdObject->mCommandQueue.Get(), 
		pDesc, 
		ppSwapChain));

	return TRUE;
}

BOOL D3D12Util::CreateSwapChain1(
		Core::Factory* const pFactory,
		Core::CommandObject* const pCmdObject,
		HWND hWnd,
		DXGI_SWAP_CHAIN_DESC1* pDesc,
		IDXGISwapChain1** ppSwapChain1) {
	CheckHRESULT(mpLogFile, pFactory->mDxgiFactory->CreateSwapChainForHwnd(
		pCmdObject->mCommandQueue.Get(),
		hWnd,
		pDesc,
		nullptr, nullptr,
		ppSwapChain1));

	return TRUE;
}

BOOL D3D12Util::CalcConstantBufferByteSize(UINT byteSize) {
	// Constant buffers must be a multiple of the minimum hardware
	// allocation size (usually 256 bytes).  So round up to nearest
	// multiple of 256.  We do this by adding 255 and then masking off
	// the lower 2 bytes which store all bits < 256.
	// Example: Suppose byteSize = 300.
	// (300 + 255) & ~255
	// 555 & ~255
	// 0x022B & ~0x00ff
	// 0x022B & 0xff00
	// 0x0200
	// 512
	return (byteSize + 255) & ~255;
}

BOOL D3D12Util::CreateDefaultBuffer(
		Core::Device* const pDevice,
		ID3D12GraphicsCommandList4* const cmdList,
		const void* const pInitData,
		UINT64 byteSize,
		ComPtr<ID3D12Resource>& uploadBuffer,
		ComPtr<ID3D12Resource>& defaultBuffer) {
	// Create the actual default buffer resource.
	CheckHRESULT(mpLogFile, pDevice->md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(defaultBuffer.GetAddressOf()))
	);

	// In order to copy CPU memory data into our default buffer, we need to create
	// an intermediate upload heap. 
	CheckHRESULT(mpLogFile, pDevice->md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(uploadBuffer.GetAddressOf()))
	);

	// Describe the data we want to copy into the default buffer.
	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = pInitData;
	subResourceData.RowPitch = byteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;

	// Schedule to copy the data to the default buffer resource.  At a high level, the helper function UpdateSubresources
	// will copy the CPU memory into the intermediate upload heap.  Then, using ID3D12CommandList::CopySubresourceRegion,
	// the intermediate upload heap data will be copied to mBuffer.
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));

	UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON));

	// Note: uploadBuffer has to be kept alive after the above function calls because
	// the command list has not been executed yet that performs the actual copy.
	// The caller can Release the uploadBuffer after it knows the copy has been executed.

	return TRUE;
}

BOOL D3D12Util::CreateUploadBuffer(
		Core::Device* const pDevice,
		UINT64 byteSize,
		const IID& riid,
		void** const ppResource) {
	if (FAILED(pDevice->md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		riid,
		ppResource))) 
		return FALSE;
	return TRUE;
}

BOOL D3D12Util::CreateBuffer(
		Core::Device* const pDevice,
		D3D12BufferCreateInfo& info,
		const IID& riid,
		void** const ppResource,
		ID3D12InfoQueue* pInfoQueue) {
	D3D12_HEAP_PROPERTIES heapDesc = {};
	heapDesc.Type = info.HeapType;
	heapDesc.CreationNodeMask = 1;
	heapDesc.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Alignment = info.Alignment;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Width = info.Size;
	resourceDesc.Flags = info.Flags;

	if (pInfoQueue != nullptr) {
		CheckHRESULT(mpLogFile, pDevice->md3dDevice->CreateCommittedResource(&heapDesc, info.HeapFlags, &resourceDesc, info.State, nullptr, riid, ppResource));
	}
	else {
		CheckHRESULT(mpLogFile, pDevice->md3dDevice->CreateCommittedResource(&heapDesc, info.HeapFlags, &resourceDesc, info.State, nullptr, riid, ppResource));
	}

	return TRUE;
}

void D3D12Util::Transite(
		Core::CommandObject* const pCmdObect,
		ID3D12Resource* const pResource,
		D3D12_RESOURCE_STATES stateBefore,
		D3D12_RESOURCE_STATES stateAfter) {
	pCmdObect->mDirectCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pResource, stateBefore, stateAfter));
}

void D3D12Util::UavBarrier(ID3D12GraphicsCommandList* const pCmdList, ID3D12Resource* pResource) {
	D3D12_RESOURCE_BARRIER uavBarrier;
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = pResource;
	uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	pCmdList->ResourceBarrier(1, &uavBarrier);
}

void D3D12Util::UavBarriers(ID3D12GraphicsCommandList* const pCmdList, ID3D12Resource* pResources[], UINT length) {
	std::vector<D3D12_RESOURCE_BARRIER> uavBarriers;
	for (UINT i = 0; i < length; ++i) {
		D3D12_RESOURCE_BARRIER uavBarrier;
		uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		uavBarrier.UAV.pResource = pResources[i];
		uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		uavBarriers.push_back(uavBarrier);
	}
	pCmdList->ResourceBarrier(static_cast<UINT>(uavBarriers.size()), uavBarriers.data());
}

void D3D12Util::UavBarrier(ID3D12GraphicsCommandList* const pCmdList, Resource::GpuResource* pResource) {
	D3D12_RESOURCE_BARRIER uavBarrier;
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = pResource->Resource();
	uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	pCmdList->ResourceBarrier(1, &uavBarrier);
}

void D3D12Util::UavBarriers(ID3D12GraphicsCommandList* const pCmdList, Resource::GpuResource* pResources[], UINT length) {
	std::vector<D3D12_RESOURCE_BARRIER> uavBarriers;
	for (UINT i = 0; i < length; ++i) {
		D3D12_RESOURCE_BARRIER uavBarrier;
		uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		uavBarrier.UAV.pResource = pResources[i]->Resource();
		uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		uavBarriers.push_back(uavBarrier);
	}
	pCmdList->ResourceBarrier(static_cast<UINT>(uavBarriers.size()), uavBarriers.data());
}

void D3D12Util::CreateShaderResourceView(
		Core::Device* const pDevice,
		ID3D12Resource* const pResource,
		const D3D12_SHADER_RESOURCE_VIEW_DESC* const pDesc,
		D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor) {
	pDevice->md3dDevice->CreateShaderResourceView(pResource, pDesc, destDescriptor);
}

void D3D12Util::CreateUnorderedAccessView(
		Core::Device* const pDevice,
		ID3D12Resource* const pResource,
		ID3D12Resource* const pCounterResource,
		const D3D12_UNORDERED_ACCESS_VIEW_DESC* const pDesc,
		D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor) {
	pDevice->md3dDevice->CreateUnorderedAccessView(pResource, pCounterResource, pDesc, destDescriptor);
}

void D3D12Util::CreateRenderTargetView(
		Core::Device* const pDevice,
		ID3D12Resource* const pResource,
		const D3D12_RENDER_TARGET_VIEW_DESC* const pDesc,
		D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor) {
	pDevice->md3dDevice->CreateRenderTargetView(pResource, pDesc, destDescriptor);
}

void D3D12Util::CreateDepthStencilView(
		Core::Device* const pDevice,
		ID3D12Resource* const pResource,
		const D3D12_DEPTH_STENCIL_VIEW_DESC* const pDesc,
		D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor) {
	pDevice->md3dDevice->CreateDepthStencilView(pResource, pDesc, destDescriptor);
}

BOOL D3D12Util::CreateTexture(
		Core::Device* const pDevice, 
		Core::CommandObject* const pCmdObject, 
		Resource::Texture* const pTexture, 
		LPCWSTR filePath,
		BOOL bGenerateMipmapIfMissing,
		UINT maxSize) {
	ResourceUploadBatch resourceUpload(pDevice->md3dDevice.Get());
	resourceUpload.Begin();
	
	const HRESULT status = DirectX::CreateDDSTextureFromFile(
		pDevice->md3dDevice.Get(),
		resourceUpload,
		filePath,
		pTexture->Resource.ReleaseAndGetAddressOf(),
		bGenerateMipmapIfMissing,
		maxSize);
	
	auto finished = resourceUpload.End(pCmdObject->mCommandQueue.Get());
	finished.wait();
	
	if (FAILED(status)) {
		std::wstringstream wsstream;
		wsstream << L"Returned 0x" << std::hex << status << L"; when creating texture:  " << filePath;
		ReturnFalse(mpLogFile, wsstream.str());
	}

	return TRUE;
}

BOOL D3D12Util::CreateRootSignature(
		Core::Device* const pDevice,
		const D3D12_ROOT_SIGNATURE_DESC& rootSignatureDesc,
		const IID& riid,
		void** const ppRootSignature,
		LPCWSTR name) {
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(
		&rootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(),
		errorBlob.GetAddressOf()
	);

	std::wstringstream wsstream;
	if (errorBlob != nullptr)
		wsstream << reinterpret_cast<char*>(errorBlob->GetBufferPointer());

	if (FAILED(hr))
		ReturnFalse(mpLogFile, wsstream.str().c_str());

	CheckHRESULT(mpLogFile, pDevice->md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		riid,
		ppRootSignature
	));
	if (name != nullptr) {
		auto rootSig = reinterpret_cast<ID3D12RootSignature*>(*ppRootSignature);
		rootSig->SetName(name);
	}

	return TRUE;
}

D3D12_GRAPHICS_PIPELINE_STATE_DESC D3D12Util::DefaultPsoDesc(D3D12_INPUT_LAYOUT_DESC inputLayout, DXGI_FORMAT dsvFormat) {
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = inputLayout;
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = dsvFormat;

	return psoDesc;
}

D3D12_GRAPHICS_PIPELINE_STATE_DESC D3D12Util::FitToScreenPsoDesc() {
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { nullptr, 0 };
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.NumRenderTargets = 1;
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;

	return psoDesc;
}

D3DX12_MESH_SHADER_PIPELINE_STATE_DESC D3D12Util::DefaultMeshPsoDesc(DXGI_FORMAT dsvFormat) {
	D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = dsvFormat;

	return psoDesc;
}

D3DX12_MESH_SHADER_PIPELINE_STATE_DESC D3D12Util::FitToScreenMeshPsoDesc() {
	D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.NumRenderTargets = 1;
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;

	return psoDesc;
}

BOOL D3D12Util::CreateComputePipelineState(
		Core::Device* const pDevice,
		const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc,
		const IID& riid,
		void** const ppPipelineState,
		LPCWSTR name) {
	CheckHRESULT(mpLogFile, pDevice->md3dDevice->CreateComputePipelineState(&desc, riid, ppPipelineState));

	if (name != nullptr) {
		auto pso = reinterpret_cast<ID3D12PipelineState*>(*ppPipelineState);
		pso->SetName(name);
	}

	return TRUE;
}

BOOL D3D12Util::CreateGraphicsPipelineState(
		Core::Device* const pDevice,
		const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc,
		const IID& riid,
		void** const ppPipelineState,
		LPCWSTR name) {
	CheckHRESULT(mpLogFile, pDevice->md3dDevice->CreateGraphicsPipelineState(&desc, riid, ppPipelineState));

	if (name != nullptr) {
		auto pso = reinterpret_cast<ID3D12PipelineState*>(*ppPipelineState);
		pso->SetName(name);
	}

	return TRUE;
}

BOOL D3D12Util::CreatePipelineState(
		Core::Device* const pDevice,
		const D3DX12_MESH_SHADER_PIPELINE_STATE_DESC& desc,
		const IID& riid,
		void** const ppPipelineState,
		LPCWSTR name) {
	auto meshStreamDesc = CD3DX12_PIPELINE_MESH_STATE_STREAM(desc);

	D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
	streamDesc.SizeInBytes = sizeof(meshStreamDesc);
	streamDesc.pPipelineStateSubobjectStream = &meshStreamDesc;

	CheckHRESULT(mpLogFile, pDevice->md3dDevice->CreatePipelineState(&streamDesc, riid, ppPipelineState));

	if (name != nullptr) {
		auto pso = reinterpret_cast<ID3D12PipelineState*>(*ppPipelineState);
		pso->SetName(name);
	}

	return TRUE;
}

BOOL D3D12Util::CreateStateObject(
		Core::Device* const pDevice,
		const D3D12_STATE_OBJECT_DESC* pDesc,
		const IID& riid,
		void** const ppStateObject) {
	CheckHRESULT(mpLogFile, pDevice->md3dDevice->CreateStateObject(pDesc, riid, ppStateObject));

	return TRUE;
}

D3D12_INPUT_LAYOUT_DESC D3D12Util::InputLayoutDesc() { return gInputLayoutDesc; }

BOOL D3D12Util::CaptureTexture(
		Core::CommandObject* const pCmdObject,
		ID3D12Resource* const pSource,
		BOOL isCubeMap,
		ScratchImage& result,
		D3D12_RESOURCE_STATES beforeState,
		D3D12_RESOURCE_STATES afterState) {
	CheckHRESULT(mpLogFile, DirectX::CaptureTexture(
		pCmdObject->mCommandQueue.Get(),
		pSource,
		isCubeMap,
		result,
		beforeState,
		afterState));

	return TRUE;
}