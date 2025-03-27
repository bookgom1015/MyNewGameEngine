#include "Render/DX/Foundation/ShadingObject.hpp"

using namespace Render::DX::Foundation;

BOOL ShadingObject::CompileShaders() { return TRUE; }

BOOL ShadingObject::BuildRootSignatures() { return TRUE; }

BOOL ShadingObject::BuildPipelineStates() { return TRUE; }

BOOL ShadingObject::BuildDescriptors(Core::DescriptorHeap* const pDescHeap) { return TRUE; }

BOOL ShadingObject::OnResize(UINT width, UINT height) { return TRUE;  }