#pragma once

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3d12.lib")

#include <map>
#include <memory>
#include <vector>

#include <wrl.h>
#include <dxgi1_6.h>

#include "Common/Render/Renderer.hpp"
#include "Render/DX/Foundation/d3dx12.h"

namespace Common::Foundation {
	struct Processor;
}

namespace Render::DX {
	namespace Foundation {
		class SwapChain;
		class DepthStencilBuffer;
	}

	class DxLowRenderer {
	protected:
		static const D3D_DRIVER_TYPE D3DDriverType = D3D_DRIVER_TYPE_HARDWARE;

	private:
		using Adapters = std::vector<std::pair<UINT, IDXGIAdapter*>>;

	protected:
		DxLowRenderer();
		virtual ~DxLowRenderer();

	public:
		RendererAPI Common::Debug::LogFile* LogFile() const;

	protected: // Functions that called only once
		virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, HWND hWnd, UINT width, UINT height);
		virtual void CleanUp();

		virtual BOOL CreateRtvAndDsvDescriptorHeaps(UINT numCbvSrvUavDescs, UINT numRtvDescs, UINT numDsvDescs);

	protected: // Functions that called by inherited class to access interferace member variables


	protected: // Functions that called whenever a message is called
		virtual BOOL OnResize(UINT width, UINT height);

	protected:
		BOOL FlushCommandQueue();

	private: // Functions that called only once to initialize DirectX
		BOOL GetHWInfo();

		BOOL InitDirect3D(UINT width, UINT height);
		BOOL SortAdapters(Adapters& adapters);
		BOOL CreateDebugObjects();
		BOOL CreateCommandObjects();
		BOOL CreateSwapChain();
		BOOL CreateDepthStencilBuffer();
		BOOL BuildDescriptors();

	protected:
		Common::Debug::LogFile* mpLogFile;

		HWND mhMainWnd = NULL;

		UINT mClientWidth = 0;
		UINT mClientHeight = 0;

		BOOL mbAllowTearing = FALSE;

		std::unique_ptr<Common::Foundation::Processor> mProcessor;
		std::unique_ptr<Foundation::SwapChain> mSwapChain;
		std::unique_ptr<Foundation::DepthStencilBuffer> mDepthStencilBuffer;

	protected:
		Microsoft::WRL::ComPtr<ID3D12Device5> md3dDevice;
		Microsoft::WRL::ComPtr<ID3D12Fence> mFence;

		// Debugging
		Microsoft::WRL::ComPtr<ID3D12Debug> mDebugController;
		DWORD mCallbakCookie = 0x01010101;

		Microsoft::WRL::ComPtr<IDXGIFactory4> mDxgiFactory;
		UINT mdxgiFactoryFlags = 0;

		// Commands
		Microsoft::WRL::ComPtr<ID3D12InfoQueue1> mInfoQueue;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> mDirectCommandList;
		std::vector<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4>> mMultiCommandLists;

		// Descriptor heaps
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mCbvSrvUavHeap;

		// Descriptor handles
		UINT mRtvDescriptorSize = 0;
		UINT mDsvDescriptorSize = 0;
		UINT mCbvSrvUavDescriptorSize = 0;

		CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuCbvSrvUav;
		CD3DX12_GPU_DESCRIPTOR_HANDLE mhGpuCbvSrvUav;
		CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuDsv;
		CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuRtv;
	};
}