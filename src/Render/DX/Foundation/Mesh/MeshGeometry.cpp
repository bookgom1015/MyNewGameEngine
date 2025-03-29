#include "Render/DX/Foundation/Mesh/MeshGeometry.hpp"

using namespace Render::DX::Foundation::Mesh;

D3D12_VERTEX_BUFFER_VIEW MeshGeometry::VertexBufferView() const {
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
	vbv.StrideInBytes = VertexByteStride;
	vbv.SizeInBytes = VertexBufferByteSize;

	return vbv;
}

D3D12_INDEX_BUFFER_VIEW MeshGeometry::IndexBufferView() const {
	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
	ibv.Format = IndexFormat;
	ibv.SizeInBytes = IndexBufferByteSize;

	return ibv;
}

void MeshGeometry::DisposeUploaders() {
	VertexBufferUploader = nullptr;
	IndexBufferUploader = nullptr;
}

Common::Foundation::Hash Render::DX::Foundation::Mesh::MeshGeometry::Hash(MeshGeometry* ptr) {
	std::hash<Render::DX::Foundation::Mesh::MeshGeometry*> hasher;
	return hasher(ptr);
}