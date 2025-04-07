#include "Render/DX/DxRenderer.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Core/HWInfo.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Foundation/Core/CommandObject.hpp"
#include "Render/DX/Foundation/Core/DescriptorHeap.hpp"
#include "Render/DX/Foundation/Resource/FrameResource.hpp"
#include "Render/DX/Foundation/Resource/MeshGeometry.hpp"
#include "Render/DX/Foundation/Util/D3D12Util.hpp"
#include "Render/DX/Foundation/Util/ShadingObjectManager.hpp"
#include "Render/DX/Shading/Util/ShaderManager.hpp"
#include "Render/DX/Shading/Util/MipmapGenerator.hpp"
#include "Render/DX/Shading/Util/SamplerUtil.hpp"
#include "Render/DX/Shading/EnvironmentMap.hpp"
#include "FrankLuna/GeometryGenerator.h"

using namespace Render::DX;
using namespace DirectX;

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
	mEnvironmentMap = std::make_unique<Shading::EnvironmentMap::EnvironmentMapClass>();
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

	return TRUE;
}

BOOL DxRenderer::Draw() {

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

BOOL DxRenderer::BuildMeshGeometry(
		Foundation::Resource::SubmeshGeometry* const submesh,
		const std::vector<Common::Foundation::Mesh::Vertex>& vertices,
		const std::vector<std::uint16_t>& indices,
		const std::string& name) {
	auto geo = std::make_unique<Foundation::Resource::MeshGeometry>();
	const auto hash = Foundation::Resource::MeshGeometry::Hash(geo.get());

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
		mShadingObjectManager->AddShadingObject(mMipmapGenerator.get());
	}
	// Environment map
	{
		auto initData = Shading::EnvironmentMap::MakeInitData();
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->ShaderManager = mShaderManager.get();
		CheckReturn(mpLogFile, mEnvironmentMap->Initialize(mpLogFile, initData.get()));
		mShadingObjectManager->AddShadingObject(mEnvironmentMap.get());
	}

	return TRUE;
}

BOOL DxRenderer::BuildFrameResources() {
	for (UINT i = 0; i < Foundation::Resource::FrameResource::Count; i++) {
		mFrameResources.push_back(std::make_unique<Foundation::Resource::FrameResource>());

		CheckReturn(mpLogFile, mFrameResources.back()->Initialize(mpLogFile, mDevice.get(), static_cast<UINT>(mProcessor->Logical), 2, 32));
	}

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

	CheckReturn(mpLogFile, BuildMeshGeometry(&sphereSubmesh, vertices, indices, "SkySphere"));

	return TRUE;
}

BOOL DxRenderer::FinishUpInitializing() {
	CheckReturn(mpLogFile, mEnvironmentMap->SetEnvironmentMap(L"./../../../../assets/textures/forest_hdr.dds"));

	return TRUE;
}