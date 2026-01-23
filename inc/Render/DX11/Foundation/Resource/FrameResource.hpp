#pragma once

#include "Render/DX11/Foundation/ConstantBuffer.h"
#include "Render/DX11/Foundation/Resource/UploadBuffer.hpp"

namespace Render::DX11::Foundation::Resource {
	class FrameResource {
	public:
		FrameResource();
		virtual ~FrameResource();

	public:
		BOOL Initalize(
			Common::Debug::LogFile* const pLogFile,
			Foundation::Core::Device* const pDevice);
		void CleanUp();

	private:
		BOOL CreateConstantBuffers();

	public:
		Common::Debug::LogFile* mpLogFile{};

		UploadBuffer<PassCB> PassCB{};
		UploadBuffer<ObjectCB> ObjectCB{};
		UploadBuffer<MaterialCB> MaterialCB{};
		UploadBuffer<LightCB> LightCB{};

	private:
		BOOL mbCleanedUp{};
		Foundation::Core::Device* mpDevice{};
	};
}