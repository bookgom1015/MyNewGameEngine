#include "Render/VK/Foundation/Core/CommandObject.hpp"
#include "Common/Debug/Logger.hpp"
#include "Render/VK/Foundation/Util/VulkanUtil.hpp"

using namespace Render::VK::Foundation::Core;

CommandObject::~CommandObject() {
	CleanUp();
}

BOOL CommandObject::Initialize(
		Common::Debug::LogFile* const pLogFile,
		VkPhysicalDevice physicalDevice,
		VkDevice device,
		VkSurfaceKHR surface,
		UINT swapChainImageCount) {
	mpLogFile = pLogFile;
	mPhysicalDevice = physicalDevice;
	mDevice = device;
	mSurface = surface;
	mSwapChainImageCount = swapChainImageCount;

	mImageAvailableSemaphores.resize(mSwapChainImageCount);
	mRenderFinishedSemaphores.resize(mSwapChainImageCount);
	mInFlightFences.resize(mSwapChainImageCount);
	mImagesInFlight.resize(mSwapChainImageCount);

	CheckReturn(mpLogFile, CreateQueues());
	CheckReturn(mpLogFile, CreateCommandPool());
	CheckReturn(mpLogFile, CreateSyncObjects());

	return TRUE;
}

void CommandObject::CleanUp() {
	for (UINT i = 0; i < mSwapChainImageCount; ++i) {
		const auto inFlightFence = mInFlightFences[i];
		const auto renderSemaphore = mRenderFinishedSemaphores[i];
		const auto imageSemaphore = mImageAvailableSemaphores[i];

		if (inFlightFence != VK_NULL_HANDLE)
			vkDestroyFence(mDevice, inFlightFence, nullptr);
		if (renderSemaphore != VK_NULL_HANDLE)
			vkDestroySemaphore(mDevice, renderSemaphore, nullptr);
		if (imageSemaphore != VK_NULL_HANDLE)
			vkDestroySemaphore(mDevice, imageSemaphore, nullptr);
	}
	if (mCommandPool != VK_NULL_HANDLE)
		vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
}

BOOL CommandObject::CreateQueues() {
	Foundation::Util::QueueFamilyIndices indices = Foundation::Util::QueueFamilyIndices::FindQueueFamilies(mPhysicalDevice, mSurface);

	vkGetDeviceQueue(mDevice, indices.GetGraphicsFamilyIndex(), 0, &mGraphicsQueue);
	vkGetDeviceQueue(mDevice, indices.GetPresentFamilyIndex(), 0, &mPresentQueue);

	return TRUE;
}

BOOL CommandObject::CreateCommandPool() {
	Foundation::Util::QueueFamilyIndices indices = Foundation::Util::QueueFamilyIndices::FindQueueFamilies(mPhysicalDevice, mSurface);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = indices.GetGraphicsFamilyIndex();
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mCommandPool) != VK_SUCCESS)
		ReturnFalse(mpLogFile, L"Failed to create command pool");

	return TRUE;
}

BOOL CommandObject::CreateSyncObjects() {
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (UINT i = 0; i < mSwapChainImageCount; ++i) {
		mImagesInFlight[i] = VK_NULL_HANDLE;

		if (vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mImageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mRenderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(mDevice, &fenceInfo, nullptr, &mInFlightFences[i]) != VK_SUCCESS)
			ReturnFalse(mpLogFile, L"Failed to create synchronization object(s) for a frame");
	}

	return TRUE;
}