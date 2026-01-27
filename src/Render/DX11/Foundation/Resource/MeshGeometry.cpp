#include "Render/DX11/Foundation/Core/pch_d3d11.h"
#include "Render/DX11/Foundation/Resource/MeshGeometry.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX11/Foundation/Core/Device.hpp"

using namespace Render::DX11::Foundation::Resource;

MeshGeometry::MeshGeometry() {}

MeshGeometry::~MeshGeometry() { CleanUp(); }

BOOL MeshGeometry::Initialize(
		Common::Debug::LogFile* const pLogFile,
		Core::Device* const pDevice,
		Common::Foundation::Mesh::Vertex const *const pVertexData, UINT vertexByteSize,
		UINT const *const pIndexData, UINT indexByteSize, 
		UINT indexCount, DXGI_FORMAT indexFormat) {
	mIndexCount = indexCount;
	mIndexFormat = indexFormat;

	// VertexBuffer
	{
		D3D11_BUFFER_DESC desc{};
		desc.Usage = D3D11_USAGE_IMMUTABLE;
		desc.ByteWidth = vertexByteSize;
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		desc.CPUAccessFlags = 0;

		D3D11_SUBRESOURCE_DATA subresource{};
		subresource.pSysMem = pVertexData;

		CheckReturn(pLogFile, pDevice->CreateBuffer(&desc, &subresource, &mVertexBuffer));
	}
	// IndexBuffer
	{
		D3D11_BUFFER_DESC desc{};
		desc.Usage = D3D11_USAGE_IMMUTABLE;
		desc.ByteWidth = indexByteSize;
		desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		desc.CPUAccessFlags = 0;

		D3D11_SUBRESOURCE_DATA subresource{};
		subresource.pSysMem = pIndexData;

		CheckReturn(pLogFile, pDevice->CreateBuffer(&desc, &subresource, &mIndexBuffer));
	}

	return TRUE;
}

void MeshGeometry::CleanUp() {
	if (mbCleanedUp) return;

	mVertexBuffer.Reset();
	mIndexBuffer.Reset();

	mbCleanedUp = TRUE;
}

Common::Foundation::Hash Render::DX11::Foundation::Resource::MeshGeometry::Hash(MeshGeometry* ptr) {
	uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
	return Common::Util::HashUtil::HashCombine(0, static_cast<Common::Foundation::Hash>(addr));
}