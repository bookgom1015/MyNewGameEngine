#pragma once

#include <array>
#include <memory>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <Windows.h>

#ifndef VK_USE_PLATFORM_WIN32_KHR
	#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

#include "Common/Util/HashUtil.hpp"

namespace Common::Debug {
	struct LogFile;
}

namespace Render::VK {
	namespace Shading::Util {
		class ShaderManager;
	}

	namespace Foundation {
		namespace Core {
			class Device;
		}

		class ShadingObject {
		public:
			virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, void* const pData);
			virtual void CleanUp();

		public:
			virtual BOOL CompileShaders();
			virtual BOOL BuildDescriptorSets();
			virtual BOOL BuildPipelineLayouts();
			virtual BOOL BuildImages();
			virtual BOOL BuildImageViews();
			virtual BOOL BuildFixedImages();
			virtual BOOL BuildFixedImageViews();
			virtual BOOL BuildRenderPass();
			virtual BOOL BuildPipelineStates();
			virtual BOOL BuildFramebuffers();
			virtual BOOL OnResize(UINT width, UINT height);

		protected:
			Common::Debug::LogFile* mpLogFile = nullptr;
		};
	}
}