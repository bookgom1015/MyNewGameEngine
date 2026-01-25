#pragma once

#include "Common/Util/HashUtil.hpp"
#include "Render/DX11/Foundation/HlslCompaction.h"

namespace Common::Debug {
	struct LogFile;
}

namespace Render::DX11 {
	namespace Foundation {
		struct RenderItem;
	}

	namespace Shading::Util {
		class ShaderManager;
	}

	namespace Foundation {
		namespace Core {
			class Device;
		}

		namespace Resource {
			class FrameResource;
		}

		class ShadingObject {
		public:
			ShadingObject();
			virtual ~ShadingObject();

		public:
			virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, void* const pData);
			virtual void CleanUp();

			virtual BOOL CompileShaders();
			virtual BOOL BuildPipelineStates();
			virtual BOOL OnResize(UINT width, UINT height);
			virtual BOOL Update();

		protected:
			BOOL mbCleanedUp{};
			Common::Debug::LogFile* mpLogFile{};
		};
	}
}