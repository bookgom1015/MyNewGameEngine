#include "Render/DX/DxRenderer.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Core/HWInfo.hpp"
#include "Common/Foundation/Camera/GameCamera.hpp"
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
#include "Render/DX/Foundation/Util/ShadingObjectManager.hpp"
#include "Render/DX/Shading/Util/ShaderManager.hpp"
#include "Render/DX/Shading/Util/MipmapGenerator.hpp"
#include "Render/DX/Shading/Util/EquirectangularConverter.hpp"
#include "Render/DX/Shading/Util/SamplerUtil.hpp"
#include "Render/DX/Shading/EnvironmentMap.hpp"
#include "FrankLuna/GeometryGenerator.h"
using namespace Render::DX;
using namespace DirectX;

namespace {
	Common::Foundation::Hash gSkySphereGeoHash;
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
	mShadingObjectManager = std::make_unique<Foundation::Util::ShadingObjectManager>();
	mShaderManager = std::make_unique<Shading::Util::ShaderManager>();

	mMipmapGenerator = std::make_unique<Shading::Util::MipmapGenerator::MipmapGeneratorClass>();
	mEquirectangularConverter = std::make_unique<Shading::Util::EquirectangularConverter::EquirectangularConverterClass>();

	mEnvironmentMap = std::make_unique<Shading::EnvironmentMap::EnvironmentMapClass>();

	mShadingObjectManager->AddShadingObject(mMipmapGenerator.get());
	mShadingObjectManager->AddShadingObject(mEquirectangularConverter.get());
	mShadingObjectManager->AddShadingObject(mEnvironmentMap.get());

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
		UINT width, UINT height) {
	CheckReturn(mpLogFile, DxLowRenderer::Initialize(pLogFile, pWndManager, width, height));

	CheckReturn(mpLogFile, InitShadingObjects());
	CheckReturn(mpLogFile, BuildFrameResources());

	CheckReturn(mpLogFile, mShadingObjectManager->CompileShaders(mShaderManager.get(), L".\\..\\..\\..\\..\\assets\\Shaders\\HLSL\\"));
	CheckReturn(mpLogFile, mShadingObjectManager->BuildRootSignatures());
	CheckReturn(mpLogFile, mShadingObjectManager->BuildPipelineStates());
	CheckReturn(mpLogFile, mShadingObjectManager->BuildDescriptors(mDescriptorHeap.get()));
	
	CheckReturn(mpLogFile, BuildSkySphere());
	
	CheckReturn(mpLogFile, mCommandObject->FlushCommandQueue());

	CheckReturn(mpLogFile, FinishUpInitializing());

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
	CheckReturn(mpLogFile, mEnvironmentMap->DrawSkySphere(
		mCurrentFrameResource,
		mSwapChain->ScreenViewport(),
		mSwapChain->ScissorRect(),
		mSwapChain->BackBuffer(),
		mSwapChain->BackBufferRtv(),
		mDepthStencilBuffer->DepthStencilBufferDsv(),
		mCurrentFrameResource->MainPassCBAddress(),
		mCurrentFrameResource->ObjectCBAddress(),
		mCurrentFrameResource->ObjectCBByteSize(),
		gSkySphereRitem));

	CheckReturn(mpLogFile, PresentAndSignal());

	return TRUE;
}

BOOL DxRenderer::AddMesh() {

	return TRUE;
}

BOOL DxRenderer::RemoveMesh() {

	return TRUE;
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
		Foundation::Resource::SubmeshGeometry* const submesh,
		const std::vector<Common::Foundation::Mesh::Vertex>& vertices,
		const std::vector<std::uint16_t>& indices,
		const std::string& name,
		Common::Foundation::Hash& hash) {
	auto geo = std::make_unique<Foundation::Resource::MeshGeometry>();
	hash = Foundation::Resource::MeshGeometry::Hash(geo.get());

	const UINT vbByteSize = static_cast<UINT>(vertices.size() * sizeof(Common::Foundation::Mesh::Vertex));
	const UINT ibByteSize = static_cast<UINT>(indices.size() * sizeof(std::uint16_t));

	mCommandObject->ResetDirectCommandList();

	CheckHRESULT(mpLogFile, D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	CheckHRESULT(mpLogFile, D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	const auto device = mDevice.get();
	const auto cmdList = mCommandObject->DirectCommandList();

	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateDefaultBuffer(
		device,
		cmdList,
		vertices.data(),
		vbByteSize,
		geo->VertexBufferUploader,
		geo->VertexBufferGPU)
	);

	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateDefaultBuffer(
		device,
		cmdList,
		indices.data(),
		ibByteSize,
		geo->IndexBufferUploader,
		geo->IndexBufferGPU)
	);

	CheckReturn(mpLogFile, mCommandObject->ExecuteDirectCommandList());
	CheckReturn(mpLogFile, mCommandObject->FlushCommandQueue());
	
	geo->VertexByteStride = static_cast<UINT>(sizeof(Common::Foundation::Mesh::Vertex));
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;	
	geo->Subsets[name] = *submesh;
	
	mMeshGeometries[hash] = std::move(geo);

	return TRUE;
}

BOOL DxRenderer::InitShadingObjects() {
	CheckReturn(mpLogFile, mShadingObjectManager->Initialize(mpLogFile));
	CheckReturn(mpLogFile, mShaderManager->Initialize(mpLogFile, static_cast<UINT>(mProcessor->Logical)));

	// MipmapGenerator
	{
		auto initData = Shading::Util::MipmapGenerator::MakeInitData();
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
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->DescriptorHeap = mDescriptorHeap.get();
		initData->ShaderManager = mShaderManager.get();
		CheckReturn(mpLogFile, mEnvironmentMap->Initialize(mpLogFile, initData.get()));
	}

	return TRUE;
}

BOOL DxRenderer::BuildFrameResources() {
	for (UINT i = 0; i < Foundation::Resource::FrameResource::Count; i++) {
		mFrameResources.push_back(std::make_unique<Foundation::Resource::FrameResource>());

		CheckReturn(mpLogFile, mFrameResources.back()->Initialize(mpLogFile, mDevice.get(), static_cast<UINT>(mProcessor->Logical), 2, 32));
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

	CheckReturn(mpLogFile, BuildMeshGeometry(&sphereSubmesh, vertices, indices, "SkySphere", gSkySphereGeoHash));

	auto ritem = std::make_unique<Foundation::RenderItem>(Foundation::Resource::FrameResource::Count);
	ritem->ObjCBIndex = static_cast<INT>(mRenderItems.size());
	ritem->Geometry = mMeshGeometries[gSkySphereGeoHash].get();
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