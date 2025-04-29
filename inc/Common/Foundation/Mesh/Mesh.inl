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

void Common::Foundation::Mesh::Mesh::Subsets(std::vector<SubsetPair>& subsets) const {
	for (const auto& subset : mSubsets) 
		subsets.emplace_back(subset.first, subset.second);
}

Common::Foundation::Mesh::Material Common::Foundation::Mesh::Mesh::GetMaterial(UINT index) const {
	return mMaterials[index];
}

#endif // __MESH_INL__