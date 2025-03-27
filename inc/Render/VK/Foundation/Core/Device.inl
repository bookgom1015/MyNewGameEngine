#ifndef __DEVICE_INL__
#define __DEVICE_INL__

VkPhysicalDevice Render::VK::Foundation::Core::Device::PhysicalDevice() const {
	return mPhysicalDevice;
}

VkDevice Render::VK::Foundation::Core::Device::LogicalDevice() const {
	return mDevice;
}

#endif // __DEVICE_INL__