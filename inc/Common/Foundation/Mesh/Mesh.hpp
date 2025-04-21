#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <Windows.h>

#include <DirectXMath.h>

#include "Common/Foundation/Mesh/Vertex.h"
#include "Common/Foundation/Mesh/Material.h"

namespace Common {
	namespace Debug {
		struct LogFile;
	}

	namespace Foundation::Mesh {
		class Mesh {
		public:
			struct Subset {
				UINT StartIndexLocation;
				UINT Size;
				INT MaterialIndex;
			};

		public:
			Mesh() = default;
			virtual ~Mesh() = default;

		public:
			__forceinline UINT VertexCount() const;
			__forceinline UINT VerticesByteSize() const;
			__forceinline const Vertex* Vertices() const;

			__forceinline UINT IndexCount() const;
			__forceinline UINT IndicesByteSize() const;
			__forceinline const UINT* Indices() const;

		public:
			static BOOL Load(
				Common::Debug::LogFile* const pLogFile,
				Mesh& mesh,
				LPCSTR fileName,
				LPCSTR baseDir,
				LPCSTR extension);

			static Common::Foundation::Hash Hash(const Mesh& mesh);

		private:
			static BOOL LoadObj(
				Common::Debug::LogFile* const pLogFile, 
				Mesh& mesh, 
				LPCSTR fileName, 
				LPCSTR baseDir,
				LPCSTR extension);
			static BOOL LoadFbx(
				Common::Debug::LogFile* const pLogFile, 
				Mesh& mesh, 
				LPCSTR fileName,
				LPCSTR baseDir,
				LPCSTR extension);

		private:
#ifdef _DEBUG
			static void DebugInfo(const Mesh& mesh);
#endif

		private:
			std::string mFilePath;

			std::unordered_map<Vertex, UINT> mUniqueVertices;
			std::vector<Vertex> mVertices;
			std::vector<UINT> mIndices;

			std::unordered_map<std::string, Subset> mSubsets;

			std::vector<Material> mMaterials;
		};
	}
}

#include "Mesh.inl"