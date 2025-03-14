#include "Common/Debug/Logger.hpp"

using namespace Common::Debug;

BOOL Logger::Initialize(LogFile* const pLogFile, LPCWSTR pFilePath) {
	pLogFile->Handle = CreateFile(
		pFilePath,
		GENERIC_WRITE,
		FILE_SHARE_WRITE,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	DWORD writtenBytes = 0;
	WORD bom = 0xFEFF;
	return WriteFile(pLogFile->Handle, &bom, 2, &writtenBytes, NULL);
}

void Logger::LogFn(LogFile* const pLogFile, const std::string& msg) {
	std::wstring wstr;
	wstr.assign(msg.begin(), msg.end());
	{
		std::lock_guard<std::mutex> lock(pLogFile->Mutex);

		DWORD writtenBytes = 0;

		WriteFile(
			pLogFile->Handle,
			wstr.c_str(),
			static_cast<DWORD>(wstr.length() * sizeof(WCHAR)),
			&writtenBytes,
			NULL
		);
	}
}

void Logger::LogFn(LogFile* const pLogFile, const std::wstring& msg) {
	std::lock_guard<std::mutex> lock(pLogFile->Mutex);

	DWORD writtenBytes = 0;

	WriteFile(
		pLogFile->Handle,
		msg.c_str(),
		static_cast<DWORD>(msg.length() * sizeof(WCHAR)),
		&writtenBytes,
		NULL
	);
}

BOOL Logger::SetTextToWnd(LogFile* const pLogFile, HWND hWnd, LPCWSTR pText) {
	CheckReturn(pLogFile, SetWindowTextW(hWnd, pText));

	return TRUE;
}

BOOL Logger::AppendTextToWnd(LogFile* const pLogFile, HWND hWnd, LPCWSTR pText) {
	const INT length = GetWindowTextLengthW(hWnd) + lstrlenW(pText) + 1;

	std::vector<WCHAR> buffer(length);

	CheckLastError(pLogFile, GetWindowTextW(hWnd, buffer.data(), length));
	wcscat_s(buffer.data(), length, pText);
	CheckReturn(pLogFile, SetWindowTextW(hWnd, buffer.data()));

	return TRUE;
}