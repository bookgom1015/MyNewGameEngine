#pragma once

#include <vector>

#include <wrl.h>

#include <d3d12.h>

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

			BOOL ExecuteDirectCommandList();
			BOOL ResetDirectCommandList();

		private:
#ifdef _DEBUG
			BOOL CreateDebugObjects();
#endif
			BOOL CreateCommandQueue();
			BOOL CreateDirectCommandObjects();
			BOOL CreateMultiCommandObjects(UINT numThreads);
			BOOL CreateFence();

		public:
			__forceinline ID3D12GraphicsCommandList4* DirectCommandList() const;

		private:
			Common::Debug::LogFile* mpLogFile = nullptr;

			Device* mDevice = nullptr;

#ifdef _DEBUG
			// Debugging
			Microsoft::WRL::ComPtr<ID3D12InfoQueue1> mInfoQueue;
			DWORD mCallbakCookie = 0x01010101;
#endif

			// Command objects
			Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
			Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> mDirectCommandList;
			std::vector<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4>> mMultiCommandLists;

			Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
			UINT64 mCurrentFence = 0;
		};
	}
}

#include "Render/DX/Foundation/Core/CommandObject.inl"