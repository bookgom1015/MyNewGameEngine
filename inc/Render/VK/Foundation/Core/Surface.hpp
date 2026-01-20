#pragma once

namespace Common::Debug {
	struct LogFile;
}

namespace Render::VK::Foundation::Core {
	class Surface {
	public:
		Surface();
		virtual ~Surface();

	public:
		BOOL Initalize(Common::Debug::LogFile* const pLogFile, HWND hWnd, VkInstance instance);
		void CleanUp();

	public:
		__forceinline VkSurfaceKHR GetSurface();

	private:
		BOOL CreateSurface();

	private:
		Common::Debug::LogFile* mpLogFile{};

		HWND mhMainWnd{};

		VkInstance mInstance{};

		VkSurfaceKHR mSurface{};
	};
}

#include "Render/VK/Foundation/Core/Surface.inl"