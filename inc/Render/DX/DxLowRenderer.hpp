#pragma once

#include "Common/Render/Renderer.hpp"

namespace Common::Foundation::Core {
	struct Processor;
}

namespace ImGuiManager::DX {
	class DxImGuiManager;
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
			Common::ImGuiManager::ImGuiManager* const pImGuiManager,
			Common::Render::ShadingArgument::ShadingArgumentSet* const pShadingArgSet,
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

		BOOL InitDirect3D();
		BOOL CreateDevice();
		BOOL CreateSwapChain();
		BOOL CreateDepthStencilBuffer();
		BOOL BuildDescriptors();

		void ReportLiveDXGIObjects();

	protected:
		UINT mClientWidth{};
		UINT mClientHeight{};

		ImGuiManager::DX::DxImGuiManager* mpImGuiManager{};
				
		std::unique_ptr<Common::Foundation::Core::Processor> mProcessor{};
		std::unique_ptr<Foundation::Core::Factory> mFactory{};
		std::unique_ptr<Foundation::Core::Device> mDevice{};
		std::unique_ptr<Foundation::Core::DescriptorHeap> mDescriptorHeap{};
		std::unique_ptr<Foundation::Core::SwapChain> mSwapChain{};
		std::unique_ptr<Foundation::Core::DepthStencilBuffer> mDepthStencilBuffer{};
		std::unique_ptr<Foundation::Core::CommandObject> mCommandObject{};
	};
}