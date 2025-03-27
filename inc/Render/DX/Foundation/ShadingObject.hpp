#pragma once

#include <wrl.h>
#include <Windows.h>

#include <dxgi1_6.h>

#include "Common/Debug/Logger.hpp"
#include "Render/DX/Foundation/Util/d3dx12.h"
#include "Render/DX/Foundation/Util/GpuResource.hpp"
#include "Render/DX/Foundation/Core/DescriptorHeap.hpp"
#include "Render/DX/Foundation/HlslCompaction.h"

namespace Render::DX::Foundation {
	class ShadingObject {
	protected:
		virtual UINT CbvSrvUavDescCount() const = 0;
		virtual UINT RtvDescCount() const = 0;
		virtual UINT DsvDescCount() const = 0;

	protected:
		virtual BOOL Initialize(void* const pData) = 0;

	protected:
		virtual BOOL CompileShaders();
		virtual BOOL BuildRootSignatures();
		virtual BOOL BuildPipelineStates();
		virtual BOOL BuildDescriptors(Core::DescriptorHeap* const pDescHeap);
		virtual BOOL OnResize(UINT width, UINT height);
	};
}