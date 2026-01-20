#pragma once

namespace Common::Debug {
	struct LogFile;
}

namespace Render::VK::Foundation::Core {
	class CommandObject {
	public:
		CommandObject();
		virtual ~CommandObject();

	public:
		__forceinline const VkCommandBuffer& CommandBuffer() const;

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
		BOOL CreateCommandBuffer();
		BOOL CreateSyncObjects();

	private:
		Common::Debug::LogFile* mpLogFile{};

		VkPhysicalDevice mPhysicalDevice{};
		VkDevice mDevice{};
		VkSurfaceKHR mSurface{};

		UINT mSwapChainImageCount{};

		// Command objects;
		VkCommandPool mCommandPool{};
		VkCommandBuffer mCommandBuffer{};

		VkQueue mGraphicsQueue{};
		VkQueue mPresentQueue{};

		// Synchronizing objects
		std::vector<VkSemaphore> mImageAvailableSemaphores{};
		std::vector<VkSemaphore> mRenderFinishedSemaphores{};
		std::vector<VkFence> mInFlightFences{};
		std::vector<VkFence> mImagesInFlight{};
		UINT mCurentImageIndex{};
		UINT mCurrentFrame{};
	};
}

namespace Render::VK::Foundation::Core {
	const VkCommandBuffer& CommandObject::CommandBuffer() const { return mCommandBuffer; }
}