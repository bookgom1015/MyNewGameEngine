#include "Render/DX11/Foundation/Core/pch_d3d11.h"
#include "Render/DX11/Foundation/ShadingObject.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/DX11/Foundation/Core/Device.hpp"
#include "Render/DX11/Foundation/Resource/FrameResource.hpp"
#include "Render/DX11/Foundation/RenderItem.hpp"

using namespace Render::DX11::Foundation;

ShadingObject::ShadingObject() {}

ShadingObject::~ShadingObject() {}

BOOL ShadingObject::Initialize(Common::Debug::LogFile* const pLogFile, void* const pData) {
	mpLogFile = pLogFile;
	NullCheck(mpLogFile, pData);

	return TRUE;
}

void ShadingObject::CleanUp() {}

BOOL ShadingObject::CompileShaders() {
	return TRUE;
}

BOOL ShadingObject::BuildPipelineStates() {
	return TRUE;
}

BOOL ShadingObject::OnResize(UINT width, UINT height) {
	return TRUE;
}

BOOL ShadingObject::Update() {
	return TRUE;
}
