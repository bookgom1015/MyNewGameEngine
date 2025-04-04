#include "Render/DX/Foundation/ShadingObject.hpp"

using namespace Render::DX::Foundation;

BOOL ShadingObject::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	mpLogFile = pLogFile;
	if (pData == nullptr) ReturnFalse(mpLogFile, L"InitData is nullptr");

	return TRUE;
}

BOOL ShadingObject::CompileShaders() { return TRUE; }

BOOL ShadingObject::BuildRootSignatures(const Render::DX::Shading::Util::StaticSamplers& samplers) { return TRUE; }

BOOL ShadingObject::BuildPipelineStates() { return TRUE; }

BOOL ShadingObject::BuildDescriptors(Core::DescriptorHeap* const pDescHeap) { return TRUE; }

BOOL ShadingObject::OnResize(UINT width, UINT height) { return TRUE;  }