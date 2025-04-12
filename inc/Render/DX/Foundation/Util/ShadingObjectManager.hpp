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

namespace Render::DX {
	namespace Shading::Util {
		class ShaderManager;
	}

	namespace Foundation {
		class ShadingObject;

		namespace Core {
			class DescriptorHeap;
		}

		namespace Util {
			class ShadingObjectManager {
			public:
				ShadingObjectManager() = default;
				virtual ~ShadingObjectManager() = default;

			public:
				BOOL Initialize(Common::Debug::LogFile* const pLogFile);

				void AddShadingObject(ShadingObject* const pShadingObject);

			public:
				UINT CbvSrvUavDescCount() const;
				UINT RtvDescCount() const;
				UINT DsvDescCount() const;

			public:
				BOOL CompileShaders(Shading::Util::ShaderManager* const pShaderManager, LPCWSTR baseDir);
				BOOL BuildRootSignatures();
				BOOL BuildPipelineStates();
				BOOL BuildDescriptors(Core::DescriptorHeap* const pDescHeap);
				BOOL OnResize(UINT width, UINT height);

			private:
				Common::Debug::LogFile* mpLogFile = nullptr;

				std::vector<ShadingObject*> mShadingObjects;
			};
		}
	}
}