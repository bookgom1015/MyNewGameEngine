#pragma once

#include <wrl.h>
#include <Windows.h>

#include <dxgi1_6.h>

#include "Common/Debug/Logger.hpp"
#include "Render/DX/Foundation/d3dx12.h"
#include "Render/DX/Foundation/GpuResource.hpp"
#include "Render/DX/Foundation/HlslCompaction.h"

namespace Render::DX::Foundation {
	class ShadingObject {
	protected:
		virtual UINT CbvSrvUavDescCount() const = 0;
		virtual UINT RtvDescCount() const = 0;
		virtual UINT DsvDescCount() const = 0;

	protected:
		virtual BOOL Initialize(void* const pData) = 0;
		virtual void CleanUp() = 0;

	protected:
		virtual BOOL CompileShaders();
		virtual BOOL BuildRootSignatures();
		virtual BOOL BuildPipelineStates();
		virtual BOOL BuildDescriptors(
			CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuCbvSrvUav, CD3DX12_GPU_DESCRIPTOR_HANDLE& hGpuCbvSrvUav,
			CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuRtv, CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuDsv,
			UINT cbvSrvUavDescSize, UINT rtvDescSize, UINT dsvDescSize);
		virtual BOOL OnResize(UINT width, UINT height);
	};
}