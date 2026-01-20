#pragma once

namespace Common::Debug {
	struct LogFile;
}

namespace Render::VK {
	namespace Shading::Util {
		class ShaderManager;
	}

	namespace Foundation {
		class ShadingObject;
	}

	namespace Shading {
		namespace Util {
			class ShadingObjectManager {
			public:
				ShadingObjectManager();
				virtual ~ShadingObjectManager();

			public:
				BOOL Initialize(Common::Debug::LogFile* const pLogFile);
				void CleanUp();

				void AddShadingObject(Foundation::ShadingObject* const pShadingObject);

			public:
				BOOL CompileShaders(Shading::Util::ShaderManager* const pShaderManager, LPCWSTR baseDir);
				BOOL BuildDescriptorSets();
				BOOL BuildPipelineLayouts();
				BOOL BuildImages();
				BOOL BuildImageViews();
				BOOL BuildFixedImages();
				BOOL BuildFixedImageViews();
				BOOL BuildRenderPass();
				BOOL BuildPipelineStates();
				BOOL BuildFramebuffers();
				BOOL OnResize(UINT width, UINT height);

			private:
				Common::Debug::LogFile* mpLogFile{};

				std::vector<Foundation::ShadingObject*> mShadingObjects{};
			};
		}
	}
}