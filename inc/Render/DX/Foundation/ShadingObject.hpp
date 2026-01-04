#pragma once

#include <memory>
#include <array>

#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <wrl.h>

#include <dxgi1_6.h>
#include <Microsoft.Direct3D.D3D12.1.615.1/build/native/include/d3dx12/d3dx12.h>

#include "Common/Util/HashUtil.hpp"
#include "Render/DX/Foundation/HlslCompaction.h"

namespace Common::Debug {
	struct LogFile;
}

namespace Render::DX {
	namespace Shading::Util {
		class ShaderManager;
	}

	namespace Foundation {
		struct RenderItem;

		namespace Core {
			class Factory;
			class Device;
			class CommandObject;
			class DescriptorHeap;
		}

		namespace Resource {
			class FrameResource;
			class GpuResource;
		}

		class ShadingObject {
		public:
			virtual UINT CbvSrvUavDescCount() const = 0;
			virtual UINT RtvDescCount() const = 0;
			virtual UINT DsvDescCount() const = 0;

		public:
			virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, void* const pData);

			virtual BOOL CompileShaders();
			virtual BOOL BuildRootSignatures();
			virtual BOOL BuildPipelineStates();
			virtual BOOL BuildDescriptors(Core::DescriptorHeap* const pDescHeap);
			virtual BOOL OnResize(UINT width, UINT height);
			virtual BOOL BuildShaderTables(UINT numRitems);
			virtual BOOL Update();

		protected:
			Common::Debug::LogFile* mpLogFile = nullptr;
		};
	}
}