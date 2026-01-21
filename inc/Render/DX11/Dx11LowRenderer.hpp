#pragma once

#include "Common/Render/Renderer.hpp"

namespace Render::DX11 {
	namespace Foundation::Core {
		class Device;
		class SwapChain;
	}

	class Dx11LowRenderer : public Common::Render::Renderer {
	public:
		Dx11LowRenderer();
		virtual ~Dx11LowRenderer();

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
		std::unique_ptr<Foundation::Core::Device> mDevice{};
		//std::unique_ptr<Foundation::Core::SwapChain> mSwapChain{};

	protected:
		UINT mClientWidth{};
		UINT mClientHeight{};
	};
}