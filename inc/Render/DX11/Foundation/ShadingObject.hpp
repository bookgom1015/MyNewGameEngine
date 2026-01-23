#pragma once

#include "Common/Util/HashUtil.hpp"
#include "Render/DX11/Foundation/ShadingConvention.h"

namespace Common::Debug {
	struct LogFile;
}

namespace Render::DX11 {
	namespace Shading::Util {
		class ShaderManager;
	}

	namespace Foundation {
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
			Common::Debug::LogFile* mpLogFile{};
		};
	}
}