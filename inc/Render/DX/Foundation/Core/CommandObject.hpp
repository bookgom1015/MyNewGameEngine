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

#include <Microsoft.Direct3D.D3D12.1.615.1/build/native/include/d3d12.h>

namespace Common::Debug {
	struct LogFile;
}

namespace Render::DX::Foundation {
	namespace Util {
		class D3D12Util;
	}

	namespace Core {
		class Device;

		class CommandObject {
		private:
			friend class Util::D3D12Util;

		public:
			CommandObject() = default;
			virtual ~CommandObject() = default;

		public: // Functions that is called only once
			BOOL Initialize(Common::Debug::LogFile* const pLogFile, Device* const pDevice, UINT numThreads);

		public: // Functions that is called whenever a renderer calls
			BOOL FlushCommandQueue();

			BOOL ResetCommandListAllocator();

			BOOL ExecuteDirectCommandList();
			BOOL ResetDirectCommandList(ID3D12PipelineState* const pPipelineState = nullptr);

			BOOL ExecuteCommandList(UINT index);
			BOOL ResetCommandList(ID3D12CommandAllocator* const pAlloc, UINT index, ID3D12PipelineState* const pPipelineState = nullptr);

			BOOL ExecuteCommandLists();
			BOOL ResetCommandLists(ID3D12CommandAllocator* const allocs[], ID3D12PipelineState* const pPipelineState = nullptr);

			BOOL WaitCompletion(UINT64 fence);
			UINT64 IncreaseFence();

			BOOL Signal();

		private:
#ifdef _DEBUG
			BOOL CreateDebugObjects();
#endif
			BOOL CreateCommandQueue();
			BOOL CreateDirectCommandObjects();
			BOOL CreateMultiCommandObjects(UINT numThreads);
			BOOL CreateFence();

		public:
			__forceinline ID3D12GraphicsCommandList6* DirectCommandList() const;
			__forceinline ID3D12GraphicsCommandList6* CommandList(UINT index) const;

			__forceinline constexpr UINT64 CurrentFence() const noexcept;

		private:
			Common::Debug::LogFile* mpLogFile = nullptr;
			Device* mpDevice = nullptr;

			UINT mThreadCount = 0;

#ifdef _DEBUG
			// Debugging
			Microsoft::WRL::ComPtr<ID3D12InfoQueue1> mInfoQueue;
			DWORD mCallbakCookie = 0x01010101;
#endif
			
			// Command objects
			Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
			Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> mDirectCommandList;
			std::vector<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6>> mMultiCommandLists;

			Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
			UINT64 mCurrentFence = 0;
		};
	}
}

#include "Render/DX/Foundation/Core/CommandObject.inl"