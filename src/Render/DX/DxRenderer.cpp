#include "Render/DX/DxRenderer.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Core/HWInfo.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Foundation/Core/CommandObject.hpp"
#include "Render/DX/Foundation/Resource/FrameResource.hpp"
#include "Render/DX/Foundation/Util/D3D12Util.hpp"
#include "Render/DX/Foundation/Resource/MeshGeometry.hpp"
#include "Render/DX/Shading/Util/ShaderManager.hpp"
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
	mShaderManager = std::make_unique<Shading::Util::ShaderManager>();

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

	CheckReturn(mpLogFile, CompileShaders());
	CheckReturn(mpLogFile, BuildRootSignatures());
	CheckReturn(mpLogFile, BuildPipelineStates());
	CheckReturn(mpLogFile, BuildDescriptors());

	CheckReturn(mpLogFile, BuildSkySphere());

	CheckReturn(mpLogFile, mCommandObject->FlushCommandQueue());

	return TRUE;
}

void DxRenderer::CleanUp() {
	DxLowRenderer::CleanUp();
}

BOOL DxRenderer::OnResize(UINT width, UINT height) {	
	CheckReturn(mpLogFile, DxLowRenderer::OnResize(width, height));

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
	CheckReturn(mpLogFile, mDescriptorHeap->CreateDescriptorHeaps(0, 0, 0));

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
	CheckReturn(mpLogFile, mShaderManager->Initialize(mpLogFile, static_cast<UINT>(mProcessor->Logical)));

	// Environment map
	{
		auto initData = Shading::EnvironmentMap::MakeInitData();
		initData->Device = mDevice.get();
		initData->CommandObject = mCommandObject.get();
		initData->ShaderManager = mShaderManager.get();
		CheckReturn(mpLogFile, mEnvironmentMap->Initialize(mpLogFile, initData.get()));
		CheckReturn(mpLogFile, mEnvironmentMap->SetEnvironmentMap(L"./../../../../assets/textures/forest_hdr.dds"));
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

BOOL DxRenderer::CompileShaders() {
	CheckReturn(mpLogFile, mEnvironmentMap->CompileShaders());
	
	CheckReturn(mpLogFile, mShaderManager->CompileShaders(L".\\..\\..\\..\\..\\assets\\Shaders\\HLSL\\"));

	return TRUE;
}

BOOL DxRenderer::BuildRootSignatures() {
	const auto& staticSamplers = Shading::Util::SamplerUtil::GetStaticSamplers();

	CheckReturn(mpLogFile, mEnvironmentMap->BuildRootSignatures(staticSamplers));

	return TRUE;
}

BOOL DxRenderer::BuildPipelineStates() {
	CheckReturn(mpLogFile, mEnvironmentMap->BuildPipelineStates());

	return TRUE;
}

BOOL DxRenderer::BuildDescriptors() {
	const auto descHeap = mDescriptorHeap.get();

	CheckReturn(mpLogFile, mEnvironmentMap->BuildDescriptors(descHeap));

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
