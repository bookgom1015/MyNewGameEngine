#include "Render/DX/Foundation/Core/pch_d3d12.h"
#include "Render/DX/Foundation/Resource/MeshGeometry.hpp"

using namespace Render::DX::Foundation::Resource;

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

Common::Foundation::Hash Render::DX::Foundation::Resource::MeshGeometry::Hash(MeshGeometry* ptr) {
	uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
	return Common::Util::HashUtil::HashCombine(0, static_cast<Common::Foundation::Hash>(addr));
}