#pragma once

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "DirectXTex.lib")

#include <map>
#include <memory>
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
#include <d3dcompiler.h>
#include <dxgi1_6.h>

#include "Common/Render/Renderer.hpp"

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

	class DxLowRenderer : public Common::Render::Renderer {
	protected:
		static const D3D_DRIVER_TYPE D3DDriverType = D3D_DRIVER_TYPE_HARDWARE;

	protected:
		DxLowRenderer();
		virtual ~DxLowRenderer();

	protected: // Functions that is called only once
		RendererAPI virtual BOOL Initialize(
			Common::Debug::LogFile* const pLogFile,
			Common::Foundation::Core::WindowsManager* const pWndManager,
			Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet,
			UINT width, UINT height) override;
		RendererAPI virtual void CleanUp() override;

	protected: // Functions that is called whenever a message is called
		RendererAPI virtual BOOL OnResize(UINT width, UINT height) override;

	public: // Functions that is called in every frame
		RendererAPI virtual BOOL Update(FLOAT deltaTime) override;
		RendererAPI virtual BOOL Draw() override;

	protected:
		virtual BOOL CreateDescriptorHeaps();

	private: // Functions that is called only once to initialize DirectX
		BOOL GetHWInfo();

		BOOL InitDirect3D(UINT width, UINT height);
		BOOL CreateDevice();
		BOOL CreateSwapChain();
		BOOL CreateDepthStencilBuffer();
		BOOL BuildDescriptors();

	protected:
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