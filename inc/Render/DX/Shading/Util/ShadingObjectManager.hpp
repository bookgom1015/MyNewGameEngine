#pragma once

namespace Common::Debug {
	struct LogFile;
}

namespace Render::DX {
	namespace Shading::Util {
		class ShaderManager;
	}

	namespace Foundation {
		class ShadingObject;

		namespace Core {
			class DescriptorHeap;
		}
	}

	namespace Shading {
		namespace Util {
			class ShadingObjectManager {
			public:
				ShadingObjectManager();
				virtual ~ShadingObjectManager();

			public:
				BOOL Initialize(Common::Debug::LogFile* const pLogFile);

				void AddShadingObject(Foundation::ShadingObject* const pShadingObject);

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

			private:
				Common::Debug::LogFile* mpLogFile{};

				std::vector<Foundation::ShadingObject*> mShadingObjects{};
			};
		}
	}
}