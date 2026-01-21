#pragma once

#include "Render/DX11/Foundation/ShadingConvention.h"

namespace Common::Debug {
	struct LogFile;
}

namespace Render::DX11::Foundation {
	class ShadingObject {
	public:
		virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, void* const pData);

		virtual BOOL CompileShaders();
		virtual BOOL BuildPipelineStates();
		virtual BOOL BuildDescriptors();
		virtual BOOL OnResize(UINT width, UINT height);
		virtual BOOL Update();

	protected:
		Common::Debug::LogFile* mpLogFile{};
	};
}