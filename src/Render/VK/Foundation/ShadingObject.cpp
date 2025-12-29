#include "Render/VK/Foundation/ShadingObject.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/VK/Foundation/Core/Device.hpp"

using namespace Render::VK::Foundation;

BOOL ShadingObject::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	mpLogFile = pLogFile;
	if (pData == nullptr) ReturnFalse(mpLogFile, L"InitData is nullptr");

	return TRUE;
}

void ShadingObject::CleanUp() {}

BOOL ShadingObject::CompileShaders() { return TRUE; }

BOOL ShadingObject::BuildDescriptorSets() { return TRUE; }

BOOL ShadingObject::BuildPipelineLayouts() { return TRUE; }

BOOL ShadingObject::BuildPipelineStates() { return TRUE; }

BOOL ShadingObject::OnResize(UINT width, UINT height) { return TRUE; }