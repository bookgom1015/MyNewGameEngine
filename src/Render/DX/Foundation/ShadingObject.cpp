#include "Render/DX/Foundation/ShadingObject.hpp"

using namespace Render::DX::Foundation;

BOOL ShadingObject::CompileShaders() { return TRUE; }

BOOL ShadingObject::BuildRootSignatures() { return TRUE; }

BOOL ShadingObject::BuildPipelineStates() { return TRUE; }

BOOL ShadingObject::BuildDescriptors(
	CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuCbvSrvUav, CD3DX12_GPU_DESCRIPTOR_HANDLE& hGpuCbvSrvUav,
	CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuRtv, CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuDsv,
	UINT cbvSrvUavDescSize, UINT rtvDescSize, UINT dsvDescSize) { return TRUE; }

BOOL ShadingObject::OnResize(UINT width, UINT height) { return TRUE;  }