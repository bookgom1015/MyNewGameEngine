#include "Render/DX/DxRenderer.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX/Foundation/Core/Device.hpp"
#include "Render/DX/Foundation/Core/CommandObject.hpp"
#include "Render/DX/Foundation/Util/D3D12Util.hpp"
#include "Render/DX/Foundation/Mesh/MeshGeometry.hpp"
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

}

DxRenderer::~DxRenderer() {
	CleanUp();
}

BOOL DxRenderer::Initialize(Common::Debug::LogFile* const pLogFile, HWND hWnd, UINT width, UINT height) {
	CheckReturn(mpLogFile, DxLowRenderer::Initialize(pLogFile, hWnd, width, height));

	CheckReturn(mpLogFile, BuildSkySphere());

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

BOOL DxRenderer::BuildMeshGeometry(
		Foundation::Mesh::SubmeshGeometry* const submesh, 
		const std::vector<Vertex>& vertices, 
		const std::vector<std::uint16_t>& indices,
		const std::string& name) {
	auto geo = std::make_unique<Foundation::Mesh::MeshGeometry>();

	const UINT vbByteSize = static_cast<UINT>(vertices.size() * sizeof(Vertex));
	const UINT ibByteSize = static_cast<UINT>(indices.size() * sizeof(std::uint16_t));

	mCommandObject->ResetDirectCommandList();

	CheckHRESULT(mpLogFile, D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	CheckHRESULT(mpLogFile, D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	const auto device = mDevice->GetDevice();
	const auto cmdList = mCommandObject->DirectCommandList();

	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateDefaultBuffer(
		mpLogFile,
		device,
		cmdList,
		vertices.data(),
		vbByteSize,
		geo->VertexBufferUploader,
		geo->VertexBufferGPU)
	);

	CheckReturn(mpLogFile, Foundation::Util::D3D12Util::CreateDefaultBuffer(
		mpLogFile,
		device,
		cmdList,
		indices.data(),
		ibByteSize,
		geo->IndexBufferUploader,
		geo->IndexBufferGPU)
	);

	CheckReturn(mpLogFile, mCommandObject->ExecuteDirectCommandList());
	CheckReturn(mpLogFile, mCommandObject->FlushCommandQueue());
	
	geo->VertexByteStride = static_cast<UINT>(sizeof(Vertex));
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;
	
	geo->Subsets[name] = *submesh;
	
	mMeshGeometries[name] = std::move(geo);

	return TRUE;
}

BOOL DxRenderer::BuildSkySphere() {
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(1.f, 32, 32);

	Foundation::Mesh::SubmeshGeometry sphereSubmesh;
	sphereSubmesh.StartIndexLocation = 0;
	sphereSubmesh.BaseVertexLocation = 0;
	
	const auto indexCount = static_cast<UINT>(sphere.GetIndices16().size());
	const auto vertexCount = static_cast<UINT>(sphere.Vertices.size());

	sphereSubmesh.IndexCount = indexCount;

	std::vector<Vertex> vertices(vertexCount);
	for (UINT i = 0, end = static_cast<UINT>(sphere.Vertices.size()); i < end; ++i) {
		const auto index = i + sphereSubmesh.BaseVertexLocation;
		vertices[index].Position = sphere.Vertices[i].Position;
		vertices[index].Normal   = sphere.Vertices[i].Normal;
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