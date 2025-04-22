#include "Render/DX/DxRenderer.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Core/HWInfo.hpp"
#include "Common/Foundation/Camera/GameCamera.hpp"
#include "Common/Render/ShadingArgument.hpp"
#include "Render/DX/Foundation/ConstantBuffer.h"
#include "Render/DX/Foundation/RenderItem.hpp"
#include "Render/DX/Foundation/Core/Factory.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Foundation/Core/CommandObject.hpp"
#include "Render/DX/Foundation/Core/DescriptorHeap.hpp"
#include "Render/DX/Foundation/Core/SwapChain.hpp"
#include "Render/DX/Foundation/Core/DepthStencilBuffer.hpp"
#include "Render/DX/Foundation/Resource/FrameResource.hpp"
#include "Render/DX/Foundation/Resource/MeshGeometry.hpp"
#include "Render/DX/Foundation/Util/D3D12Util.hpp"
#include "Render/DX/Shading/Util/ShadingObjectManager.hpp"
#include "Render/DX/Shading/Util/ShaderManager.hpp"
#include "Render/DX/Shading/Util/MipmapGenerator.hpp"
#include "Render/DX/Shading/Util/EquirectangularConverter.hpp"
#include "Render/DX/Shading/Util/SamplerUtil.hpp"
#include "Render/DX/Shading/EnvironmentMap.hpp"
#include "Render/DX/Shading/GammaCorrection.hpp"
#include "Render/DX/Shading/ToneMapping.hpp"
#include "Render/DX/Shading/GBuffer.hpp"
#include "FrankLuna/GeometryGenerator.h"
using namespace Render::DX;
using namespace DirectX;

namespace {
	Foundation::RenderItem* gSkySphereRitem;
}

extern "C" RendererAPI Common::Render::Renderer* Render::CreateRenderer() {
	return new DxRenderer();
}

extern "C" RendererAPI void Render::DestroyRenderer(Common::Render::Renderer* const renderer) {
	delete renderer;
}

DxRenderer::DxRenderer() {
	// Shading objets
	mShadingObjectManager = std::make_unique<Shading::Util::ShadingObjectManager>();
	mShaderManager = std::make_unique<Shading::Util::ShaderManager>();

	mMipmapGenerator = std::make_unique<Shading::Util::MipmapGenerator::MipmapGeneratorClass>();
	mEquirectangularConverter = std::make_unique<Shading::Util::EquirectangularConverter::EquirectangularConverterClass>();

	mEnvironmentMap = std::make_unique<Shading::EnvironmentMap::EnvironmentMapClass>();
	mGammaCorrection = std::make_unique<Shading::GammaCorrection::GammaCorrectionClass>();
	mToneMapping = std::make_unique<Shading::ToneMapping::ToneMappingClass>();
	mGBuffer = std::make_unique<Shading::GBuffer::GBufferClass>();

	mShadingObjectManager->AddShadingObject(mMipmapGenerator.get());
	mShadingObjectManager->AddShadingObject(mEquirectangularConverter.get());
	mShadingObjectManager->AddShadingObject(mEnvironmentMap.get());
	mShadingObjectManager->AddShadingObject(mGammaCorrection.get());
	mShadingObjectManager->AddShadingObject(mToneMapping.get());
	mShadingObjectManager->AddShadingObject(mGBuffer.get());

	// Constant buffers
	mMainPassCB = std::make_unique<ConstantBuffers::PassCB>();
	mEquirectConvCB = std::make_unique<ConstantBuffers::EquirectangularConverterCB>();
}

DxRenderer::~DxRenderer() {
	CleanUp();
}

BOOL DxRenderer::Initialize(
		Common::Debug::LogFile* const pLogFile, 
		Common::Foundation::Core::WindowsManager* const pWndManager, 
		Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet,
		UINT width, UINT height) {
	CheckReturn(mpLogFile, DxLowRenderer::Initialize(pLogFile, pWndManager, pArgSet, width, height));

	CheckReturn(mpLogFile, InitShadingObjects());
	CheckReturn(mpLogFile, BuildFrameResources());

	CheckReturn(mpLogFile, mShadingObjectManager->CompileShaders(mShaderManager.get(), L".\\..\\..\\..\\..\\assets\\Shaders\\HLSL\\"));
	CheckReturn(mpLogFile, mShadingObjectManager->BuildRootSignatures());
	CheckReturn(mpLogFile, mShadingObjectManager->BuildPipelineStates());
	CheckReturn(mpLogFile, mShadingObjectManager->BuildDescriptors(mDescriptorHeap.get()));

	CheckReturn(mpLogFile, BuildSkySphere());
	CheckReturn(mpLogFile, FinishUpInitializing());

	CheckReturn(mpLogFile, mCommandObject->FlushCommandQueue());

	return TRUE;
}

void DxRenderer::CleanUp() {
	DxLowRenderer::CleanUp();
}

BOOL DxRenderer::OnResize(UINT width, UINT height) {
	CheckReturn(mpLogFile, DxLowRenderer::OnResize(width, height));

	CheckReturn(mpLogFile, mShadingObjectManager->OnResize(width, height));

#ifdef _DEBUG
	std::cout << "DxRenderer resized (Width: " << width << " Height: " << height << ")" << std::endl;
#endif

	return TRUE;
}

BOOL DxRenderer::Update(FLOAT deltaTime) {
	mCurrentFrameResourceIndex = (mCurrentFrameResourceIndex + 1) % Foundation::Resource::FrameResource::Count;
	mCurrentFrameResource = mFrameResources[mCurrentFrameResourceIndex].get();
	CheckReturn(mpLogFile, mCommandObject->WaitCompletion(mCurrentFrameResource->mFence));
	
	CheckReturn(mpLogFile, UpdateConstantBuffers());

	return TRUE;
}

BOOL DxRenderer::Draw() {
	CheckReturn(mpLogFile, mCurrentFrameResource->ResetCommandListAllocators());

	CheckReturn(mpLogFile, mEnvironmentMap->DrawSkySphere(
		mCurrentFrameResource,
		mSwapChain->ScreenViewport(),
		mSwapChain->ScissorRect(),
		mToneMapping->InterMediateMapResource(),
		mToneMapping->InterMediateMapRtv(),
		mDepthStencilBuffer->DepthStencilBufferDsv(),
		mCurrentFrameResource->MainPassCBAddress(),
		mCurrentFrameResource->ObjectCBAddress(),
		mCurrentFrameResource->ObjectCBByteSize(),
		gSkySphereRitem));

	CheckReturn(mpLogFile, mGammaCorrection->ApplyCorrection(
		mCurrentFrameResource,
		mSwapChain->ScreenViewport(),
		mSwapChain->ScissorRect(),
		mToneMapping->InterMediateMapResource(),
		mToneMapping->InterMediateMapRtv(),
		mpArgumentSet->GammaCorrection.Gamma));

	CheckReturn(mpLogFile, mToneMapping->Resolve(
		mCurrentFrameResource,
		mSwapChain->ScreenViewport(),
		mSwapChain->ScissorRect(),
		mSwapChain->BackBuffer(),
		mSwapChain->BackBufferRtv(),
		mpArgumentSet->ToneMapping.Exposure));
	
	CheckReturn(mpLogFile, PresentAndSignal());

	return TRUE;
}

BOOL DxRenderer::AddMesh(Common::Foundation::Mesh::Mesh* const pMesh, Common::Foundation::Hash& hash) {
	Foundation::Resource::MeshGeometry* meshGeo;
	{
		CheckReturn(mpLogFile, mCommandObject->ResetCommandList(
			mCurrentFrameResource->CommandAllocator(0),
			0));
		const auto CmdList = mCommandObject->CommandList(0);

		CheckReturn(mpLogFile, BuildMeshGeometry(CmdList, pMesh, meshGeo));

		CheckReturn(mpLogFile, mCommandObject->ExecuteCommandList(0));
	}

	for (const auto& subset : meshGeo->Subsets) {
		auto ritem = std::make_unique<Foundation::RenderItem>(Foundation::Resource::FrameResource::Count);
		hash = Foundation::RenderItem::Hash(ritem.get());

		ritem->ObjCBIndex = static_cast<INT>(mRenderItems.size());
		ritem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		ritem->Geometry = meshGeo;
		ritem->IndexCount = meshGeo->Subsets[subset.first].IndexCount;
		ritem->StartIndexLocation = meshGeo->Subsets[subset.first].StartIndexLocation;
		ritem->BaseVertexLocation = meshGeo->Subsets[subset.first].BaseVertexLocation;

		mRenderItems.push_back(std::move(ritem));
	}

	return TRUE;
}

void DxRenderer::RemoveMesh(Common::Foundation::Hash hash) {

}

BOOL DxRenderer::CreateDescriptorHeaps() {
	UINT cbvSrvUavCount = mShadingObjectManager->CbvSrvUavDescCount();
	UINT rtvCount = mShadingObjectManager->RtvDescCount();
	UINT dsvCount = mShadingObjectManager->DsvDescCount();

	CheckReturn(mpLogFile, mDescriptorHeap->CreateDescriptorHeaps(cbvSrvUavCount, rtvCount, dsvCount));

	return TRUE;
}

BOOL DxRenderer::UpdateConstantBuffers() {
	CheckReturn(mpLogFile, UpdateMainPassCB());
	CheckReturn(mpLogFile, UpdateObjectCB());
	CheckReturn(mpLogFile, UpdateEquirectangularConverterCB());

	return TRUE;
}

BOOL DxRenderer::UpdateMainPassCB() {
	if (mpCamera == nullptr) return TRUE;

	// Transform NDC space [-1 , +1]^2 to texture space [0, 1]^2
	const XMMATRIX T(
		0.5f, 0.f,  0.f, 0.f,
		0.f, -0.5f, 0.f, 0.f,
		0.f,  0.f,  1.f, 0.f,
		0.5f, 0.5f, 0.f, 1.f
	);

	const XMMATRIX view = XMLoadFloat4x4(&mpCamera->View());
	const XMMATRIX proj = XMLoadFloat4x4(&mpCamera->Proj());
	const XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	
	const XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	const XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	const XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);
	
	const XMMATRIX viewProjTex = XMMatrixMultiply(viewProj, T);
	
	mMainPassCB->PrevViewProj = mMainPassCB->ViewProj;
	XMStoreFloat4x4(&mMainPassCB->View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB->InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB->Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB->InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB->ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB->InvViewProj, XMMatrixTranspose(invViewProj));
	XMStoreFloat4x4(&mMainPassCB->ViewProjTex, XMMatrixTranspose(viewProjTex));
	XMStoreFloat3(&mMainPassCB->EyePosW, mpCamera->Position());

	//const size_t offsetIndex = static_cast<size_t>(mCommandObject->CurrentFence()) % mFittedToBakcBufferHaltonSequence.size();
	//mMainPassCB->JitteredOffset = bTaaEnabled ? mFittedToBakcBufferHaltonSequence[offsetIndex] : XMFLOAT2(0.f, 0.f);
	
	mCurrentFrameResource->CopyMainPassCB(0, *mMainPassCB.get());

	return TRUE;
}

BOOL DxRenderer::UpdateObjectCB() {
	for (auto& ritem : mRenderItems) {
		// Only update the cbuffer data if the constants have changed.  
		// This needs to be tracked per frame resource.
		if (ritem->NumFramesDirty > 0) {
			const XMMATRIX world = XMLoadFloat4x4(&ritem->World);
			const XMMATRIX texTransform = XMLoadFloat4x4(&ritem->TexTransform);

			ConstantBuffers::ObjectCB objCB;
			objCB.PrevWorld = ritem->PrevWolrd;
			XMStoreFloat4x4(&objCB.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objCB.TexTransform, XMMatrixTranspose(texTransform));

			ritem->PrevWolrd = objCB.World;

			mCurrentFrameResource->CopyObjecCB(ritem->ObjCBIndex, objCB);

			// Next FrameResource need to be updated too.
			ritem->NumFramesDirty--;
		}
	}

	return TRUE;
}

BOOL DxRenderer::UpdateEquirectangularConverterCB() {
	XMStoreFloat4x4(&mEquirectConvCB->Proj, XMMatrixTranspose(XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.f, 0.1f, 10.f)));

	// Positive +X
	XMStoreFloat4x4(
		&mEquirectConvCB->View[0],
		XMMatrixTranspose(XMMatrixLookAtLH(
			XMVectorSet(0.f, 0.f, 0.f, 1.f),
			XMVectorSet(1.f, 0.f, 0.f, 1.f),
			XMVectorSet(0.f, 1.f, 0.f, 0.f)
		))
	);
	// Positive -X
	XMStoreFloat4x4(
		&mEquirectConvCB->View[1],
		XMMatrixTranspose(XMMatrixLookAtLH(
			XMVectorSet(0.f, 0.f, 0.f, 1.f),
			XMVectorSet(-1.f, 0.f, 0.f, 1.f),
			XMVectorSet(0.f, 1.f, 0.f, 0.f)
		))
	);
	// Positive +Y
	XMStoreFloat4x4(
		&mEquirectConvCB->View[2],
		XMMatrixTranspose(XMMatrixLookAtLH(
			XMVectorSet(0.f, 0.f, 0.f, 1.f),
			XMVectorSet(0.f, 1.f, 0.f, 1.f),
			XMVectorSet(0.f, 0.f, -1.f, 0.f)
		))
	);
	// Positive -Y
	XMStoreFloat4x4(
		&mEquirectConvCB->View[3],
		XMMatrixTranspose(XMMatrixLookAtLH(
			XMVectorSet(0.f, 0.f, 0.f, 1.f),
			XMVectorSet(0.f, -1.f, 0.f, 1.f),
			XMVectorSet(0.f, 0.f, 1.f, 0.f)
		))
	);
	// Positive +Z
	XMStoreFloat4x4(
		&mEquirectConvCB->View[4],
		XMMatrixTranspose(XMMatrixLookAtLH(
			XMVectorSet(0.f, 0.f, 0.f, 1.f),
			XMVectorSet(0.f, 0.f, 1.f, 1.f),
			XMVectorSet(0.f, 1.f, 0.f, 0.f)
		))
	);
	// Positive -Z
	XMStoreFloat4x4(
		&mEquirectConvCB->View[5],
		XMMatrixTranspose(XMMatrixLookAtLH(
			XMVectorSet(0.f, 0.f, 0.f, 1.f),
			XMVectorSet(0.f, 0.f, -1.f, 1.f),
			XMVectorSet(0.f, 1.f, 0.f, 0.f)
		))
	);

	mCurrentFrameResource->CopyEquirectConvCB(0, *mEquirectConvCB.get());

	return TRUE;
}

BOOL DxRenderer::BuildMeshGeometry(
		ID3D12GraphicsCommandList6* const pCmdList,
		Foundation::Resource::SubmeshGeometry* const pSubmesh,
		const std::vector<Common::Foundation::Mesh::Vertex>& vertices,
		const std::vector<std::uint16_t>& indices,
		const std::string& name,
		Foundation::Resource::MeshGeometry*& pMeshGeo) {
	auto geo = std::make_unique<Foundation::Resource::MeshGeometry>();
	const auto Hash = Foundation::Resource::MeshGeometry::Hash(geo.get());

	const UINT VerticesByteSize = static_cast<UINT>(vertices.size() * sizeof(Common::Foundation::Mesh::Vertex));
	const UINT IndicesByteSize = static_cast<UINT>(indices.size() * sizeof(std::uint16_t));

	CheckHRESULT(mpLogFile, D3DCreateBlob(VerticesByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), VerticesByteSize);

	CheckHRESULT(mpLogFile, D3DCreateBlob(IndicesByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), IndicesByteSize);

	const auto device = mDevice.get();

	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateDefaultBuffer(
		device,
		pCmdList,
		vertices.data(),
		VerticesByteSize,
		geo->VertexBufferUploader,
		geo->VertexBufferGPU)
	);

	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateDefaultBuffer(
		device,
		pCmdList,
		indices.data(),
		IndicesByteSize,
		geo->IndexBufferUploader,
		geo->IndexBufferGPU)
	);
	
	geo->VertexByteStride = static_cast<UINT>(sizeof(Common::Foundation::Mesh::Vertex));
	geo->VertexBufferByteSize = VerticesByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = IndicesByteSize;
	geo->Subsets[name] = *pSubmesh;
	
	pMeshGeo = geo.get();
	mMeshGeometries[Hash] = std::move(geo);

	return TRUE;
}

BOOL DxRenderer::BuildMeshGeometry(
		ID3D12GraphicsCommandList6* const pCmdList, 
		Common::Foundation::Mesh::Mesh* const pMesh,
		Foundation::Resource::MeshGeometry*& pMeshGeo) {
	auto geo = std::make_unique<Foundation::Resource::MeshGeometry>();
	const auto Hash = Foundation::Resource::MeshGeometry::Hash(geo.get());

	const UINT VerticesByteSize = pMesh->VerticesByteSize();
	const UINT IndicesByteSize = pMesh->IndicesByteSize();

	const auto Vertices = pMesh->Vertices();
	const auto Indices = pMesh->Indices();

	CheckHRESULT(mpLogFile, D3DCreateBlob(VerticesByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), Vertices, VerticesByteSize);

	CheckHRESULT(mpLogFile, D3DCreateBlob(IndicesByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), Indices, IndicesByteSize);

	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateDefaultBuffer(
		mDevice.get(),
		pCmdList,
		Vertices,
		VerticesByteSize,
		geo->VertexBufferUploader,
		geo->VertexBufferGPU)
	);
	
	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateDefaultBuffer(
		mDevice.get(),
		pCmdList,
		Indices,
		IndicesByteSize,
		geo->IndexBufferUploader,
		geo->IndexBufferGPU)
	);

	const auto Fence = mCommandObject->IncreaseFence();
	mCurrentFrameResource->mFence = Fence;

	geo->VertexByteStride = static_cast<UINT>(sizeof(Common::Foundation::Mesh::Vertex));
	geo->VertexBufferByteSize = VerticesByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = IndicesByteSize;
	geo->Fence = Fence;

	std::vector<Common::Foundation::Mesh::Mesh::SubsetPair> subsets;
	pMesh->Subsets(subsets);

	for (const auto& subset : subsets) {
		Foundation::Resource::SubmeshGeometry submesh;
		submesh.StartIndexLocation = subset.second.StartIndexLocation;
		submesh.BaseVertexLocation = 0;
		submesh.IndexCount = subset.second.Size;
		geo->Subsets[subset.first] = submesh;
	}

	pMeshGeo = geo.get();
	mMeshGeometries[Hash] = std::move(geo);

	return TRUE;
}

BOOL DxRenderer::InitShadingObjects() {
	CheckReturn(mpLogFile, mShadingObjectManager->Initialize(mpLogFile));
	CheckReturn(mpLogFile, mShaderManager->Initialize(mpLogFile, static_cast<UINT>(mProcessor->Logical)));

	// MipmapGenerator
	{
		auto initData = Shading::Util::MipmapGenerator::MakeInitData();
		initData->MeshShaderSupported = mbMeshShaderSupported;
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->DescriptorHeap = mDescriptorHeap.get();
		initData->ShaderManager = mShaderManager.get();
		CheckReturn(mpLogFile, mMipmapGenerator->Initialize(mpLogFile, initData.get()));
	}
	// EquirectangularConverter
	{
		auto initData = Shading::Util::EquirectangularConverter::MakeInitData();
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->DescriptorHeap = mDescriptorHeap.get();
		initData->ShaderManager = mShaderManager.get();
		CheckReturn(mpLogFile, mEquirectangularConverter->Initialize(mpLogFile, initData.get()));
	}
	// EnvironmentMap
	{
		auto initData = Shading::EnvironmentMap::MakeInitData();
		initData->MeshShaderSupported = mbMeshShaderSupported;
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->DescriptorHeap = mDescriptorHeap.get();
		initData->ShaderManager = mShaderManager.get();
		CheckReturn(mpLogFile, mEnvironmentMap->Initialize(mpLogFile, initData.get()));
	}
	// GammaCorrection
	{
		auto initData = Shading::GammaCorrection::MakeInitData();
		initData->MeshShaderSupported = mbMeshShaderSupported;
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->DescriptorHeap = mDescriptorHeap.get();
		initData->ShaderManager = mShaderManager.get();
		initData->ClientWidth = mClientWidth;
		initData->ClientHeight = mClientHeight;
		CheckReturn(mpLogFile, mGammaCorrection->Initialize(mpLogFile, initData.get()));
	}
	// ToneMapping
	{
		auto initData = Shading::ToneMapping::MakeInitData();
		initData->MeshShaderSupported = mbMeshShaderSupported;
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->DescriptorHeap = mDescriptorHeap.get();
		initData->ShaderManager = mShaderManager.get();
		initData->ClientWidth = mClientWidth;
		initData->ClientHeight = mClientHeight;
		CheckReturn(mpLogFile, mToneMapping->Initialize(mpLogFile, initData.get()));
	}
	// GBuffer
	{
		auto initData = Shading::GBuffer::MakeInitData();
		initData->MeshShaderSupported = mbMeshShaderSupported;
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->DescriptorHeap = mDescriptorHeap.get();
		initData->ShaderManager = mShaderManager.get();
		initData->ClientWidth = mClientWidth;
		initData->ClientHeight = mClientHeight;
		CheckReturn(mpLogFile, mGBuffer->Initialize(mpLogFile, initData.get()));
	}

	return TRUE;
}

BOOL DxRenderer::BuildFrameResources() {
	for (UINT i = 0; i < Foundation::Resource::FrameResource::Count; i++) {
		mFrameResources.push_back(std::make_unique<Foundation::Resource::FrameResource>());

		CheckReturn(mpLogFile, mFrameResources.back()->Initialize(mpLogFile, mDevice.get(), static_cast<UINT>(mProcessor->Logical), 2, 32, 32));
	}

	mCurrentFrameResourceIndex = 0;
	mCurrentFrameResource = mFrameResources[mCurrentFrameResourceIndex].get();

	return TRUE;
}

BOOL DxRenderer::BuildSkySphere() {
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(1.f, 32, 32);

	Foundation::Resource::SubmeshGeometry sphereSubmesh;
	sphereSubmesh.StartIndexLocation = 0;
	sphereSubmesh.BaseVertexLocation = 0;

	const auto indexCount = static_cast<UINT>(sphere.GetIndices16().size());
	const auto vertexCount = static_cast<UINT>(sphere.Vertices.size());

	sphereSubmesh.IndexCount = indexCount;

	std::vector<Common::Foundation::Mesh::Vertex> vertices(vertexCount);
	for (UINT i = 0, end = static_cast<UINT>(sphere.Vertices.size()); i < end; ++i) {
		const auto index = i + sphereSubmesh.BaseVertexLocation;
		vertices[index].Position = sphere.Vertices[i].Position;
		vertices[index].Normal = sphere.Vertices[i].Normal;
		vertices[index].TexCoord = sphere.Vertices[i].TexC;
	}

	std::vector<std::uint16_t> indices(indexCount);
	for (UINT i = 0, end = static_cast<UINT>(sphere.GetIndices16().size()); i < end; ++i) {
		const auto index = i + sphereSubmesh.StartIndexLocation;
		indices[index] = sphere.GetIndices16()[i];
	}

	auto ritem = std::make_unique<Foundation::RenderItem>(Foundation::Resource::FrameResource::Count);

	CheckReturn(mpLogFile, mCommandObject->ResetDirectCommandList());

	const auto CmdList = mCommandObject->DirectCommandList();
	CheckReturn(mpLogFile, BuildMeshGeometry(CmdList, &sphereSubmesh, vertices, indices, "SkySphere", ritem->Geometry));

	CheckReturn(mpLogFile, mCommandObject->ExecuteDirectCommandList());

	ritem->ObjCBIndex = static_cast<INT>(mRenderItems.size());
	ritem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	ritem->IndexCount = ritem->Geometry->Subsets["SkySphere"].IndexCount;
	ritem->StartIndexLocation = ritem->Geometry->Subsets["SkySphere"].StartIndexLocation;
	ritem->BaseVertexLocation = ritem->Geometry->Subsets["SkySphere"].BaseVertexLocation;
	XMStoreFloat4x4(&ritem->World, XMMatrixScaling(1000.f, 1000.f, 1000.f));

	gSkySphereRitem = ritem.get();
	mRenderItems.push_back(std::move(ritem));

	return TRUE;
}

BOOL DxRenderer::FinishUpInitializing() {
	// Some of shading objects require particular constant buffers
	CheckReturn(mpLogFile, UpdateConstantBuffers());

	CheckReturn(mpLogFile, mEnvironmentMap->SetEnvironmentMap(
		mMipmapGenerator.get(), 
		mEquirectangularConverter.get(), 
		mCurrentFrameResource->EquirectConvCBAddress(),
		L"./../../../../assets/textures/forest_hdr.dds"));

	return TRUE;
}

BOOL DxRenderer::PresentAndSignal() {
	CheckReturn(mpLogFile, mSwapChain->ReadyToPresent(mCurrentFrameResource));
	CheckReturn(mpLogFile, mSwapChain->Present(mFactory->AllowTearing()));
	mSwapChain->NextBackBuffer();
	
	mCurrentFrameResource->mFence = mCommandObject->IncreaseFence();

	CheckReturn(mpLogFile, mCommandObject->Signal());

	return TRUE;
}