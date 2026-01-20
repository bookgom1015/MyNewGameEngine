#pragma once

namespace Common::Debug {
	struct LogFile;
}

namespace Render::VK::Foundation::Core {
	class Instance {
	public:
		Instance();
		virtual ~Instance();

	public:
		BOOL Initalize(Common::Debug::LogFile* const pLogFile);
		void CleanUp();

	public:
		__forceinline VkInstance GetInstance() const;

	private:
		BOOL CreateInstance();

	private:
		Common::Debug::LogFile* mpLogFile{};

		VkInstance mInstance{};

		VkDebugUtilsMessengerEXT mDebugMessenger{};
	};
}

#include "Render/VK/Foundation/Core/Instance.inl"