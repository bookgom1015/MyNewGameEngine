#pragma once

#include "Render/DX/Foundation/ShadingObject.hpp"

namespace Common::Debug {
	struct LogFile;
}

namespace Render::DX {
	namespace Shading::Util {
		class ShaderManager;
	}

	namespace Foundation::Core {
		class DescriptorHeap;
	}

	namespace Shading {
		namespace Util {
			class ShadingObjectManager {
			public:
				ShadingObjectManager();
				virtual ~ShadingObjectManager();

			public:
				UINT CbvSrvUavDescCount() const;
				UINT RtvDescCount() const;
				UINT DsvDescCount() const;

			public:
				BOOL CompileShaders(Shading::Util::ShaderManager* const pShaderManager, LPCWSTR baseDir);
				BOOL BuildRootSignatures();
				BOOL BuildPipelineStates();
				BOOL BuildDescriptors(Foundation::Core::DescriptorHeap* const pDescHeap);
				BOOL OnResize(UINT width, UINT height);
				BOOL BuildShaderTables(UINT numRitems);
				BOOL Update();

			public:
				BOOL Initialize(Common::Debug::LogFile* const pLogFile);
				void CleanUp();

				template <typename T>
					requires std::is_base_of_v<Foundation::ShadingObject, T>
				__forceinline void Add();

				template <typename T>
					requires std::is_base_of_v<Foundation::ShadingObject, T>
				__forceinline T* Get();

			private:
				BOOL mbCleanedUp{};
				Common::Debug::LogFile* mpLogFile{};

				std::vector<std::unique_ptr<Foundation::ShadingObject>> mShadingObjects{};
				std::unordered_map<std::type_index, Foundation::ShadingObject*> mShadingObjectRefs{};
			};
		}
	}
}

#include "ShadingObjectManager.inl"