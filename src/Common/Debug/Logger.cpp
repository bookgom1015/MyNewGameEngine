#include "Common/Debug/Logger.hpp"

using namespace Debug;

HANDLE Logger::sLogFile = CreateFile(
	L"./log.txt",
	GENERIC_WRITE,
	FILE_SHARE_WRITE,
	NULL,
	CREATE_ALWAYS,
	FILE_ATTRIBUTE_NORMAL,
	NULL
);

std::mutex Logger::sLogFileMutex;

BOOL Logger::Initialize() {
	DWORD writtenBytes = 0;
	WORD bom = 0xFEFF;
	return WriteFile(Logger::sLogFile, &bom, 2, &writtenBytes, NULL);
}

void Debug::Logger::LogFn(const std::string& msg) {
	std::wstring wstr;
	wstr.assign(msg.begin(), msg.end());
	{
		std::lock_guard<std::mutex> lock(Logger::sLogFileMutex);

		DWORD writtenBytes = 0;

		WriteFile(
			Logger::sLogFile,
			wstr.c_str(),
			static_cast<DWORD>(wstr.length() * sizeof(WCHAR)),
			&writtenBytes,
			NULL
		);
	}
}

void Debug::Logger::LogFn(const std::wstring& msg) {
	std::lock_guard<std::mutex> lock(Logger::sLogFileMutex);

	DWORD writtenBytes = 0;

	WriteFile(
		Logger::sLogFile,
		msg.c_str(),
		static_cast<DWORD>(msg.length() * sizeof(WCHAR)),
		&writtenBytes,
		NULL
	);
}

BOOL Debug::Logger::SetTextToWnd(HWND hWnd, LPCWSTR text) {
	CheckReturn(SetWindowTextW(hWnd, text));

	return TRUE;
}

BOOL Debug::Logger::AppendTextToWnd(HWND hWnd, LPCWSTR text) {
	const INT length = GetWindowTextLengthW(hWnd) + lstrlenW(text) + 1;

	std::vector<WCHAR> buffer(length);

	CheckLastError(GetWindowTextW(hWnd, buffer.data(), length));
	wcscat_s(buffer.data(), length, text);
	CheckReturn(SetWindowTextW(hWnd, buffer.data()));

	return TRUE;
}