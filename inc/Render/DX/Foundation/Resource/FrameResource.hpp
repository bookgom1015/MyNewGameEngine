#pragma once

#include <vector>

#include <wrl.h>
#include <Windows.h>

#include "Render/DX/Foundation/ConstantBuffer.h"
#include "Render/DX/Foundation/Resource/UploadBuffer.hpp"

namespace Common::Debug {
	struct LogFile;
}

namespace Render::DX::Foundation {
	namespace Core {
		class Device;
	}

	namespace Resource {
		class FrameResource {
		public:
			static const UINT Count = 3;

		public:
			FrameResource() = default;
			virtual ~FrameResource() = default;

		public:
			BOOL Initialize(
				Common::Debug::LogFile* const pLogFile, 
				Core::Device* const pDevice,
				UINT numThreads,
				UINT numPasses, 
				UINT numObjects);

		private:
			BOOL CreateCommandListAllocators();
			BOOL BuildConstantBuffres();

		private:
			Common::Debug::LogFile* mpLogFile = nullptr;
			Core::Device* mpDevice = nullptr;

			std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> mCmdAllocators;

			UploadBuffer<ConstantBuffers::PassCB> mPassCB;
			UINT mPassCount = 0;

			UploadBuffer<ConstantBuffers::ObjectCB> mObjectCB;
			UINT mObjectCount = 0;
		};
	}
}