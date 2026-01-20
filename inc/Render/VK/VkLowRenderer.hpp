#pragma once

#include "Common/Render/Renderer.hpp"

namespace Common::Foundation::Core {
	struct Processor;
}

namespace ImGuiManager::VK {
	class VkImGuiManager;
}

namespace Render::VK {
	namespace Foundation::Core {
		class Instance;
		class Surface;
		class Device;
		class SwapChain;
		class CommandObject;
	}

	class VkLowRenderer : public Common::Render::Renderer {
	protected:
		VkLowRenderer();
		virtual ~VkLowRenderer();

	protected: // Functions that is called only once
		RendererAPI virtual BOOL Initialize(
			Common::Debug::LogFile* const pLogFile,
			Common::Foundation::Core::WindowsManager* const pWndManager,
			Common::ImGuiManager::ImGuiManager* const pImGuiManager,
			Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet,
			UINT width, UINT height) override;
		RendererAPI virtual void CleanUp() override;

	protected: // Functions that is called whenever a message is called
		RendererAPI virtual BOOL OnResize(UINT width, UINT height) override;

	public: // Functions that is called in every frame
		RendererAPI virtual BOOL Update(FLOAT deltaTime) override;
		RendererAPI virtual BOOL Draw() override;

	private:
		BOOL GetHWInfo();

		BOOL CreateInstance();
		BOOL CreateSurface();
		BOOL CreateDevice();
		BOOL CreateSwapChain();
		BOOL CreateCommandObjects();

	protected:
		Common::Debug::LogFile* mpLogFile{};

		ImGuiManager::VK::VkImGuiManager* mpImGuiManager{};

		UINT mClientWidth{};
		UINT mClientHeight{};

	protected:
		std::unique_ptr<Common::Foundation::Core::Processor> mProcessor{};

		std::unique_ptr<Foundation::Core::Instance> mInstance{};
		std::unique_ptr<Foundation::Core::Surface> mSurface{};
		std::unique_ptr<Foundation::Core::Device> mDevice{};
		std::unique_ptr<Foundation::Core::SwapChain> mSwapChain{};
		std::unique_ptr<Foundation::Core::CommandObject> mCommandObject{};
	};
}