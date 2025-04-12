#pragma once

#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <Windows.h>

#ifndef VK_USE_PLATFORM_WIN32_KHR
	#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

namespace Common::Debug {
	struct LogFile;
}

namespace Render::VK::Foundation::Core {
	class CommandObject {
	public:
		CommandObject() = default;
		virtual ~CommandObject();

	public:
		BOOL Initialize(
			Common::Debug::LogFile* const pLogFile, 
			VkPhysicalDevice physicalDevice, 
			VkDevice device, 
			VkSurfaceKHR surface,
			UINT swapChainImageCount);
		void CleanUp();

	private:
		BOOL CreateQueues();
		BOOL CreateCommandPool();
		BOOL CreateSyncObjects();

	private:
		Common::Debug::LogFile* mpLogFile;

		VkPhysicalDevice mPhysicalDevice;
		VkDevice mDevice;
		VkSurfaceKHR mSurface;

		UINT mSwapChainImageCount = 0;

		// Command objects;
		VkCommandPool mCommandPool;

		VkQueue mGraphicsQueue;
		VkQueue mPresentQueue;

		// Synchronizing objects
		std::vector<VkSemaphore> mImageAvailableSemaphores;
		std::vector<VkSemaphore> mRenderFinishedSemaphores;
		std::vector<VkFence> mInFlightFences;
		std::vector<VkFence> mImagesInFlight;
		UINT mCurentImageIndex = 0;
		UINT mCurrentFrame = 0;
	};
}