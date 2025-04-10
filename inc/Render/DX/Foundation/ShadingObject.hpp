#pragma once

#include <array>
#include <memory>

#include <wrl.h>
#include <Windows.h>

#include <dxgi1_6.h>

#include "Common/Debug/Logger.hpp"
#include "Common/Foundation/Util/HashUtil.hpp"
#include "Render/DX/Foundation/Util/d3dx12.h"
#include "Render/DX/Foundation/HlslCompaction.h"
#include "Render/DX/Shading/Util/SamplerUtil.hpp"

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
			virtual BOOL BuildRootSignatures(const Render::DX::Shading::Util::StaticSamplers& samplers);
			virtual BOOL BuildPipelineStates();
			virtual BOOL BuildDescriptors(Core::DescriptorHeap* const pDescHeap);
			virtual BOOL OnResize(UINT width, UINT height);

		protected:
			Common::Debug::LogFile* mpLogFile = nullptr;
		};
	}
}