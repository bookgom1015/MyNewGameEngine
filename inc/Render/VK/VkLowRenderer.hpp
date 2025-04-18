#pragma once

#pragma comment(lib, "vulkan-1.lib")

#include <array>
#include <memory>

#ifndef VK_USE_PLATFORM_WIN32_KHR
	#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

#include "Common/Render/Renderer.hpp"

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
			Common::Render::ShadingArgument::ShadingArgumentSet* const pArgSet,
			UINT width, UINT height) override;
		RendererAPI virtual void CleanUp() override;

	protected: // Functions that is called whenever a message is called
		RendererAPI virtual BOOL OnResize(UINT width, UINT height) override;

	public: // Functions that is called in every frame
		RendererAPI virtual BOOL Update(FLOAT deltaTime) override;
		RendererAPI virtual BOOL Draw() override;

	private:
		BOOL CreateInstance();
		BOOL CreateSurface();
		BOOL CreateDevice();
		BOOL CreateSwapChain();
		BOOL CreateCommandObjects();

	protected:
		Common::Debug::LogFile* mpLogFile = nullptr;

		UINT mClientWidth = 0;
		UINT mClientHeight = 0;

	protected:
		std::unique_ptr<Foundation::Core::Instance> mInstance;
		std::unique_ptr<Foundation::Core::Surface> mSurface;
		std::unique_ptr<Foundation::Core::Device> mDevice;
		std::unique_ptr<Foundation::Core::SwapChain> mSwapChain;
		std::unique_ptr<Foundation::Core::CommandObject> mCommandObject;
	};
}