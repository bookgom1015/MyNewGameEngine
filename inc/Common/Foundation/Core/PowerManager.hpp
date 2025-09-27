#pragma once

#pragma comment(lib, "PowrProf.lib")

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <Windows.h>

namespace Common {
	namespace Debug {
		struct LogFile;
	}

	namespace Foundation::Core {
		class PowerManager {
		public:
			PowerManager() = default;
			virtual ~PowerManager();

		public:
			BOOL Initialize(Common::Debug::LogFile* const pLogFile);

			BOOL SetMode(BOOL mode);
			__forceinline BOOL IsSleepable() const;

		private:
			BOOL PreventSleep();
			void AllowSleep();

		private:
			Common::Debug::LogFile* mpLogFile;

			HANDLE mhPowerRequest;
			BOOL mbSleepable = TRUE;
		};
	}	
}

#include "PowerManager.inl"