#pragma once

#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <wrl.h>
#include <Windows.h>

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
				ShadingObjectManager() = default;
				virtual ~ShadingObjectManager() = default;

			public:
				BOOL Initialize(Common::Debug::LogFile* const pLogFile);

				void AddShadingObject(Foundation::ShadingObject* const pShadingObject);

			public:
				BOOL CompileShaders(Shading::Util::ShaderManager* const pShaderManager, LPCWSTR baseDir);
				BOOL BuildDescriptorSets();
				BOOL BuildPipelineLayouts();
				BOOL BuildRenderPass();
				BOOL BuildPipelineStates();
				BOOL BuildFramebuffers();
				BOOL OnResize(UINT width, UINT height);

			private:
				Common::Debug::LogFile* mpLogFile = nullptr;

				std::vector<Foundation::ShadingObject*> mShadingObjects;
			};
		}
	}
}