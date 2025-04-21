#ifndef __MESH_INL__
#define __MESH_INL__

UINT Common::Foundation::Mesh::Mesh::VertexCount() const {
	return static_cast<UINT>(mVertices.size());
}

UINT Common::Foundation::Mesh::Mesh::VerticesByteSize() const {
	return static_cast<UINT>(mVertices.size() * sizeof(Vertex));
}

const Common::Foundation::Mesh::Vertex* Common::Foundation::Mesh::Mesh::Vertices() const {
	return mVertices.data();
}

UINT Common::Foundation::Mesh::Mesh::IndexCount() const {
	return static_cast<UINT>(mIndices.size());
}

UINT Common::Foundation::Mesh::Mesh::IndicesByteSize() const {
	return static_cast<UINT>(mIndices.size() * sizeof(UINT));
}

const UINT* Common::Foundation::Mesh::Mesh::Indices() const {
	return mIndices.data();
}

#endif // __MESH_INL__