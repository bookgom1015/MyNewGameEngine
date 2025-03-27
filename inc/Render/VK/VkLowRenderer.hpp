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

	class VkLowRenderer {
	protected:
		VkLowRenderer();
		virtual ~VkLowRenderer();

	protected: // Functions that is called only once
		virtual BOOL Initialize(Common::Debug::LogFile* const pLogFile, HWND hWnd, UINT width, UINT height);
		virtual void CleanUp();

	protected: // Functions that is called whenever a message is called
		virtual BOOL OnResize(UINT width, UINT height);

	private:
		BOOL CreateInstance();
		BOOL CreateSurface();
		BOOL CreateDevice();
		BOOL CreateSwapChain();
		BOOL CreateCommandObjects();

	protected:
		Common::Debug::LogFile* mpLogFile = nullptr;

		HWND mhMainWnd = NULL;

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