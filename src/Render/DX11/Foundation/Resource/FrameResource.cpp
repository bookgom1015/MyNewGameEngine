#include "Render/DX11/Foundation/Core/pch_d3d11.h"
#include "Render/DX11/Foundation/Resource/FrameResource.hpp"
#include "Common/Debug/Logger.hpp"

using namespace Render::DX11::Foundation::Resource;

FrameResource::FrameResource() {}

FrameResource::~FrameResource() {}

BOOL FrameResource::Initalize(
		Common::Debug::LogFile* const pLogFile,
		Foundation::Core::Device* const pDevice) {
	mpLogFile = pLogFile;
	mpDevice = pDevice;

	CheckReturn(mpLogFile, CreateConstantBuffers());

	return TRUE;
}

void FrameResource::CleanUp() {
	if (mbCleanedUp) return;

	PassCB.CleanUp();
	ObjectCB.CleanUp();
	MaterialCB.CleanUp();
	LightCB.CleanUp();

	mbCleanedUp = TRUE;
}

BOOL FrameResource::CreateConstantBuffers() {
	CheckReturn(mpLogFile, PassCB.Initialize(mpLogFile, mpDevice, 1));
	CheckReturn(mpLogFile, ObjectCB.Initialize(mpLogFile, mpDevice, 32));
	CheckReturn(mpLogFile, MaterialCB.Initialize(mpLogFile, mpDevice, 32));
	CheckReturn(mpLogFile, LightCB.Initialize(mpLogFile, mpDevice, MAX_LIGHTS));
	CheckReturn(mpLogFile, GBufferCB.Initialize(mpLogFile, mpDevice, 1));

	return TRUE;
}