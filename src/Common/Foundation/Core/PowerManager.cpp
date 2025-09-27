#include "Common/Foundation/Core/PowerManager.hpp"
#include "Common/Debug/Logger.hpp"

#include <powrprof.h>

using namespace Common::Foundation::Core;

PowerManager::~PowerManager() {	AllowSleep(); }

BOOL PowerManager::Initialize(Common::Debug::LogFile* const pLogFile) {
	mpLogFile = pLogFile;

	return TRUE;
}

BOOL PowerManager::SetMode(BOOL mode) {
	if (mode) {
		return PreventSleep();
	}
	else {
		AllowSleep();
		return TRUE;
	}	
}

BOOL PowerManager::PreventSleep() {
	if (!mbSleepable) return TRUE;

	REASON_CONTEXT context = {};
	context.Version = POWER_REQUEST_CONTEXT_VERSION;
	context.Flags = POWER_REQUEST_CONTEXT_SIMPLE_STRING;
	context.Reason.SimpleReasonString = const_cast<wchar_t*>(L"MyGameEngine is running");

	mhPowerRequest = PowerCreateRequest(&context);
	if (mhPowerRequest == NULL) ReturnFalse(mpLogFile, L"Failed to create PowerCreateRequest");

	BOOL ok = TRUE;
	ok &= PowerSetRequest(mhPowerRequest, PowerRequestDisplayRequired);
	ok &= PowerSetRequest(mhPowerRequest, PowerRequestSystemRequired);

	if (!ok) ReturnFalse(mpLogFile, L"PowerSetRequest Failed");

	mbSleepable = FALSE;

	return TRUE;
}

void PowerManager::AllowSleep() {
	if (!mhPowerRequest) return;

	PowerClearRequest(mhPowerRequest, PowerRequestDisplayRequired);
	PowerClearRequest(mhPowerRequest, PowerRequestSystemRequired);

	CloseHandle(mhPowerRequest);
	mhPowerRequest = NULL;
	mbSleepable = TRUE;
}