#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <Windows.h>

#include <DirectXMath.h>

#include "Common/Foundation/Mesh/Vertex.hpp"
#include "Common/Foundation/Mesh/Material.hpp"

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
			static BOOL LoadObj(
				Common::Debug::LogFile* const pLogFile, 
				Mesh& mesh, 
				const std::string& filePath, 
				const std::string& baseDir);
			static BOOL LoadFbx(
				Common::Debug::LogFile* const pLogFile, 
				Mesh& mesh, 
				const std::string& filePath, 
				const std::string& baseDir);

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