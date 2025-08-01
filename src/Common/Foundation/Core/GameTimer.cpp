#include "Common/Foundation/Core/GameTimer.hpp"

using namespace Common::Foundation::Core;

namespace{
	const FLOAT FrameTime30f = 1.f / 30.f;
	const FLOAT FrameTime60f = 1.f / 60.f;
	const FLOAT FrameTime90f = 1.f / 90.f;
	const FLOAT FrameTime120f = 1.f / 120.f;
	const FLOAT FrameTime144f = 1.f / 144.f;
	const FLOAT FrameTime244f = 1.f / 244.f;
}

GameTimer::GameTimer() {
	INT64 countsPerSec;
	QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&countsPerSec));
	mSecondsPerCount = 1. / static_cast<double>(countsPerSec);
}

// Returns the total time elapsed since Reset() was called, NOT counting any
// time when the clock is stopped.
FLOAT GameTimer::TotalTime() const {
	// If we are stopped, do not count the time that has passed since we stopped.
	// Moreover, if we previously already had a pause, the distance 
	// mStopTime - mBaseTime includes paused time, which we do not want to count.
	// To correct this, we can subtract the paused time from mStopTime:  
	//
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------------*------> time
	//  mBaseTime       mStopTime        startTime     mStopTime    mCurrTime
	if (mStopped) return static_cast<FLOAT>(((mStopTime - mPausedTime) - mBaseTime) * mSecondsPerCount);
	// The distance mCurrTime - mBaseTime includes paused time,
	// which we do not want to count.  To correct this, we can subtract 
	// the paused time from mCurrTime:  
	//
	//  (mCurrTime - mPausedTime) - mBaseTime 
	//
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------> time
	//  mBaseTime       mStopTime        startTime     mCurrTime	
	else return static_cast<FLOAT>(((mCurrTime - mPausedTime) - mBaseTime) * mSecondsPerCount);
}

FLOAT GameTimer::DeltaTime() const {
	return static_cast<FLOAT>(mDeltaTime);
}

void GameTimer::Reset() {
	INT64 currTime;
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currTime));

	mBaseTime = currTime;
	mPrevTime = currTime;
	mStopTime = 0;
	mStopped = FALSE;
}

void GameTimer::Start() {
	INT64 startTime;
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&startTime));

	// Accumulate the time elapsed between stop and start pairs.
	//
	//                     |<-------d------->|
	// ----*---------------*-----------------*------------> time
	//  mBaseTime       mStopTime        startTime     
	if (mStopped) {
		mPausedTime += (startTime - mStopTime);
		mPrevTime = startTime;
		mStopTime = 0;
		mStopped = FALSE;
	}
}

void GameTimer::Stop() {
	if (!mStopped) {
		INT64 currTime;
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currTime));

		mStopTime = currTime;
		mStopped = TRUE;
	}
}

BOOL GameTimer::Tick() {
	if (mStopped) {
		mDeltaTime = 0.;
		return FALSE;
	}

	INT64 currTime;
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currTime));
	mCurrTime = currTime;

	const DOUBLE delta = (mCurrTime - mPrevTime) * mSecondsPerCount;

	// Time difference between this frame and the previous.
	mDeltaTime = delta;

	BOOL status = FALSE;

	if (delta >= FrameTimeLimit()) {
		// Prepare for next frame.
		mPrevTime = mCurrTime;

		status = TRUE;
	}

	// Force nonnegative.  The DXSDK's CDXUTTimer mentions that if the 
	// processor goes into a power save mode or we get shuffled to another
	// processor, then mDeltaTime can be negative.
	if (delta < 0.) mDeltaTime = 0.;

	return status;
}

constexpr FLOAT GameTimer::FrameTimeLimit() const {
	switch (mFrameTimeLimit) {
	case FrameTimeLimits::E_30f: return FrameTime30f;
	case FrameTimeLimits::E_60f: return FrameTime60f;
	case FrameTimeLimits::E_90f: return FrameTime90f;
	case FrameTimeLimits::E_120f: return FrameTime120f;
	case FrameTimeLimits::E_144f: return FrameTime144f;
	case FrameTimeLimits::E_244f: return FrameTime244f;
	case FrameTimeLimits::E_Unlimited:
	default: return 0;
	}
}

void GameTimer::SetFrameTimeLimit(FrameTimeLimits limit) {
	mFrameTimeLimit = limit;
}