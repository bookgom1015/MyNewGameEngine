#pragma once

#ifndef VK_USE_PLATFORM_WIN32_KHR
	#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

namespace Common::Debug {
	struct LogFile;
}

namespace Render::VK::Foundation::Core {
	class Instance {
	public:
		Instance() = default;
		virtual ~Instance();

	public:
		BOOL Initalize(Common::Debug::LogFile* const pLogFile);
		void CleanUp();

	public:
		__forceinline VkInstance GetInstance() const;

	private:
		BOOL CreateInstance();

	private:
		Common::Debug::LogFile* mpLogFile = nullptr;

		VkInstance mInstance;

		VkDebugUtilsMessengerEXT mDebugMessenger;
	};
}

#include "Render/VK/Foundation/Core/Instance.inl"