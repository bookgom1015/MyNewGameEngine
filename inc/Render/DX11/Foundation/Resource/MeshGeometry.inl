#ifndef __MESHGEOMETRY_INL__
#define __MESHGEOMETRY_INL__

namespace Render::DX11::Foundation::Resource {
	ID3D11Buffer** MeshGeometry::VertexBufferAddress() noexcept { return mVertexBuffer.GetAddressOf(); }

	ID3D11Buffer* MeshGeometry::IndexBufferAddress() noexcept { return mIndexBuffer.Get(); }

	constexpr UINT MeshGeometry::IndexCount() const noexcept { return mIndexCount; }
}

#endif // __MESHGEOMETRY_INL__