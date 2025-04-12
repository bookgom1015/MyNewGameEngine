#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <wrl.h>
#include <Windows.h>

namespace Common::Foundation::Core {
	class GameTimer {
	public:
		enum FrameTimeLimits {
			E_Unlimited,
			E_30f,
			E_60f,
			E_90f,
			E_120f,
			E_144f,
			E_244f
		};
	public:
		GameTimer();
		virtual ~GameTimer() = default;

	public:
		FLOAT TotalTime() const; // in seconds
		FLOAT DeltaTime() const; // in seconds

		void Reset(); // Call before message loop.
		void Start(); // Call when unpaused.
		void Stop();  // Call when paused.
		void Tick();  // Call every frame.

		constexpr FLOAT FrameTimeLimit() const;
		void SetFrameTimeLimit(FrameTimeLimits limit);

	private:
		DOUBLE mSecondsPerCount = 0.;
		DOUBLE mDeltaTime = -1.;

		INT64 mBaseTime = 0;
		INT64 mPausedTime = 0;
		INT64 mStopTime = 0;
		INT64 mPrevTime = 0;
		INT64 mCurrTime = 0;

		BOOL mStopped = FALSE;

		FrameTimeLimits mFrameTimeLimit;
	};
}