#pragma once

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include <map>
#include <memory>
#include <vector>

#include <wrl.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

#include "Common/Render/Renderer.hpp"
#include "Render/DX/Foundation/Util/d3dx12.h"

namespace Common::Foundation::Core {
	struct Processor;
}

namespace Render::DX {
	namespace Foundation::Core {
		class Factory;
		class Device;
		class DescriptorHeap;
		class SwapChain;
		class DepthStencilBuffer;
		class CommandObject;
	}

	class DxLowRenderer {
	protected:
		static const D3D_DRIVER_TYPE D3DDriverType = D3D_DRIVER_TYPE_HARDWARE;

	protected:
		DxLowRenderer();
		virtual ~DxLowRenderer();

	protected: // Functions that is called only once
		virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, HWND hWnd, UINT width, UINT height);
		virtual void CleanUp();

		virtual BOOL CreateDescriptorHeaps();

	protected: // Functions that is called whenever a message is called
		virtual BOOL OnResize(UINT width, UINT height);

	private: // Functions that is called only once to initialize DirectX
		BOOL GetHWInfo();

		BOOL InitDirect3D(UINT width, UINT height);
		BOOL CreateSwapChain();
		BOOL CreateDepthStencilBuffer();
		BOOL BuildDescriptors();

	protected:
		Common::Debug::LogFile* mpLogFile = nullptr;

		HWND mhMainWnd = NULL;

		UINT mClientWidth = 0;
		UINT mClientHeight = 0;
				
		std::unique_ptr<Common::Foundation::Core::Processor> mProcessor;
		std::unique_ptr<Foundation::Core::Factory> mFactory;
		std::unique_ptr<Foundation::Core::Device> mDevice;
		std::unique_ptr<Foundation::Core::DescriptorHeap> mDescriptorHeap;
		std::unique_ptr<Foundation::Core::SwapChain> mSwapChain;
		std::unique_ptr<Foundation::Core::DepthStencilBuffer> mDepthStencilBuffer;
		std::unique_ptr<Foundation::Core::CommandObject> mCommandObject;
	};
}