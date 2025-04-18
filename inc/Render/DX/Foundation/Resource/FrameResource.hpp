#pragma once

#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
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
			__forceinline ID3D12CommandAllocator* CommandAllocator(UINT index) const;
			__forceinline void CommandAllocators(std::vector<ID3D12CommandAllocator*>& allocs) const;

			__forceinline D3D12_GPU_VIRTUAL_ADDRESS MainPassCBAddress() const;
			UINT MainPassCBByteSize() const;
			
			__forceinline D3D12_GPU_VIRTUAL_ADDRESS ObjectCBAddress() const;
			UINT ObjectCBByteSize() const;

			__forceinline D3D12_GPU_VIRTUAL_ADDRESS EquirectConvCBAddress() const;
			UINT EquirectConvCBByteSize() const;

		public:
			BOOL Initialize(
				Common::Debug::LogFile* const pLogFile, 
				Core::Device* const pDevice,
				UINT numThreads,
				UINT numPasses, 
				UINT numObjects);

		public:
			BOOL ResetCommandListAllocators();

		public:
			__forceinline void CopyMainPassCB(INT elementIndex, const ConstantBuffers::PassCB& data);
			__forceinline void CopyObjecCB(INT elementIndex, const ConstantBuffers::ObjectCB& data);
			__forceinline void CopyEquirectConvCB(INT elementIndex, const ConstantBuffers::EquirectangularConverterCB& data);

		private:
			BOOL CreateCommandListAllocators();
			BOOL BuildConstantBuffres(
				UINT numPasses,
				UINT numObjects);

		public:
			UINT64 mFence = 0;

		private:
			Common::Debug::LogFile* mpLogFile = nullptr;
			Core::Device* mpDevice = nullptr;

			std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> mCmdAllocators;

			UINT mThreadCount = 0;

			UploadBuffer<ConstantBuffers::PassCB> mMainPassCB;
			UploadBuffer<ConstantBuffers::ObjectCB> mObjectCB;
			UploadBuffer<ConstantBuffers::EquirectangularConverterCB> mEquirectConvCB;
		};
	}
}

#include "Render/DX/Foundation/Resource/FrameResource.inl"