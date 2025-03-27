#include "Common/Foundation/Mesh/Mesh.hpp"
#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Mesh/Material.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tinyobjloader/tiny_obj_loader.h"

using namespace Common::Foundation::Mesh;
using namespace DirectX;

BOOL Mesh::LoadObj(Common::Debug::LogFile* const pLogFile, Mesh& mesh, const std::string& filePath, const std::string& baseDir) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filePath.c_str(), baseDir.c_str())) {
		std::wstringstream wsstream;
		wsstream << err.c_str();
		ReturnFalse(pLogFile, wsstream.str());
	}

	for (const auto& shape : shapes) {
		UINT startIndexLocation = static_cast<UINT>(mesh.mIndices.size());
		UINT indexCount = 0;		
		INT prevMatId = -1;

		for (const auto& index : shape.mesh.indices) {
			Vertex vertex = {};
			
			vertex.Position = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.Normal = {
				attrib.normals[3 * index.normal_index + 0],
				attrib.normals[3 * index.normal_index + 1],
				attrib.normals[3 * index.normal_index + 2]
			};

			vertex.TexCoord = { 
				attrib.texcoords[2 * index.texcoord_index + 0], 
				attrib.texcoords[2 * index.texcoord_index + 1]
			};

			if (mesh.mUniqueVertices.count(vertex) == 0) {
				mesh.mUniqueVertices[vertex] = static_cast<UINT>(mesh.mVertices.size());
				mesh.mVertices.push_back(vertex);
			}

			mesh.mIndices.push_back(static_cast<UINT>(mesh.mUniqueVertices[vertex]));
			++indexCount;

			INT currMatId = static_cast<INT>(shape.mesh.material_ids[index.vertex_index]);
			if (prevMatId != currMatId) {
				if (prevMatId != -1) {
					Subset subset;
					subset.StartIndexLocation = startIndexLocation;
					subset.Size = indexCount;
					subset.MaterialIndex = prevMatId;
					mesh.mSubsets[materials[prevMatId].name] = subset;
				}
				startIndexLocation = static_cast<UINT>(mesh.mIndices.size());
				indexCount = 0;
				prevMatId = currMatId;
			}
		}
	}

	
	for (const auto& material : materials) {
		Material mat;
		mat.Name = material.name;
		mat.Albedo = XMFLOAT4(material.diffuse[0], material.diffuse[1], material.diffuse[2], 1.f);
		
		if (!material.diffuse_texname.empty()) mat.DiffuseMap = material.diffuse_texname;

		if (!material.normal_texname.empty()) mat.NormalMap = material.normal_texname;
		
		if (material.alpha_texname.empty()) mat.Alpha = material.dissolve;
		else mat.AlphaMap = material.alpha_texname;

		if (material.roughness_texname.empty()) mat.Roughness = material.roughness;
		else mat.RoughnessMap = material.roughness_texname;

		if (!material.metallic_texname.empty()) mat.MetalnessMap = material.metallic_texname;

		if (material.specular_texname.empty()) mat.Specular = XMFLOAT3(material.specular[0], material.specular[1], material.specular[2]);
		else mat.SpecularMap = material.specular_texname;

		mesh.mMaterials.push_back(mat);
	}

#ifdef _DEBUG
	DebugInfo(mesh);
#endif

	return TRUE;
}

BOOL Mesh::LoadFbx(Common::Debug::LogFile* const pLogFile, Mesh& mesh, const std::string& filePath, const std::string& baseDir) {
	return TRUE;
}

#ifdef _DEBUG
void Mesh::DebugInfo(const Mesh& mesh) {
	std::cout << "Mesh Info:" << std::endl;
	std::cout << "    Vertex count: " << mesh.mVertices.size() << std::endl;
	std::cout << "    Index count: " << mesh.mIndices.size() << std::endl;
	std::cout << "    Subset count: " << mesh.mSubsets.size() << std::endl;
	for (const auto& subset : mesh.mSubsets) {
		std::cout << "        Subset: " << subset.first << std::endl;
		std::cout << "            Start index location: " << subset.second.StartIndexLocation << std::endl;
		std::cout << "            Size: " << subset.second.Size << std::endl;
		std::cout << "            Material index: " << subset.second.MaterialIndex << std::endl;
	}
	std::cout << "    Material count: " << mesh.mMaterials.size() << std::endl;
	for (const auto& material : mesh.mMaterials) {
		std::cout << "        Material: " << material.Name << std::endl;
		std::cout << "            Diffuse map: " << material.DiffuseMap << std::endl;
		std::cout << "            Albedo: " << material.Albedo.x << ", " << material.Albedo.y << ", " << material.Albedo.z << ", " << material.Albedo.w << std::endl;
		std::cout << "            Normal map: " << material.NormalMap << std::endl;
		std::cout << "            Alpha map: " << material.AlphaMap << std::endl;
		std::cout << "            Alpha: " << material.Alpha << std::endl;
		std::cout << "            Roughness map: " << material.RoughnessMap << std::endl;
		std::cout << "            Roughness: " << material.Roughness << std::endl;
		std::cout << "            Metalness map: " << material.MetalnessMap << std::endl;
		std::cout << "            Metalness: " << material.Metalness << std::endl;
		std::cout << "            Specular map: " << material.SpecularMap << std::endl;
		std::cout << "            Specular: " << material.Specular.x << ", " << material.Specular.y << ", " << material.Specular.z << std::endl;
	}

}
#endif