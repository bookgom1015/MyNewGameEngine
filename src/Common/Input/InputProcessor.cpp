#include "Common/Input/InputProcessor.hpp"

using namespace Common::Input;

BOOL InputProcessor::Initialize(Common::Debug::LogFile* const pLogFile) {
	mpLogFile = pLogFile;

	return TRUE;
}

void InputProcessor::Halt() {
	bDestroying = TRUE;
	PostQuitMessage(0);
}