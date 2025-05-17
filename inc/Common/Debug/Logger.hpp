#pragma once

#include <exception>
#include <iostream>
#include <mutex>
#include <string>
#include <sstream>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <Windows.h>

#ifndef Log
#define Log(__logfile, __x, ...) {								\
	std::vector<std::string> _msgs = { __x, __VA_ARGS__ };		\
	std::stringstream _sstream;									\
	for (const auto& _msg : _msgs) _sstream << _msg;			\
	Common::Debug::Logger::LogFn(__logfile, _sstream.str());	\
}
#endif // Log

#ifndef Logln
#define Logln(__logfile, __x, ...) {							\
	std::vector<std::string> _msgs = { __x, __VA_ARGS__ };		\
	std::stringstream _sstream;									\
	for (const auto& _msg : _msgs) _sstream << _msg;			\
	_sstream << '\n';											\
	Common::Debug::Logger::LogFn(__logfile, _sstream.str());	\
}
#endif // Logln

#ifndef WLog
#define WLog(__logfile, __x, ...) {								\
	std::vector<std::wstring> _msgs = { __x, __VA_ARGS__ };		\
	std::wstringstream _wsstream;								\
	for (const auto& _msg : _msgs) _wsstream << _msg;			\
	Common::Debug::Logger::LogFn(__logfile, _wsstream.str());	\
}
#endif // WLog

#ifndef WLogln
#define WLogln(__logfile, __x, ...) {							\
	std::vector<std::wstring> _msgs = { __x, __VA_ARGS__ };		\
	std::wstringstream _wsstream;								\
	for (const auto& _msg : _msgs) _wsstream << _msg;			\
	_wsstream << L'\n';											\
	Common::Debug::Logger::LogFn(__logfile, _wsstream.str());	\
}
#endif // WLogln

#ifndef ReturnFalse
#define ReturnFalse(__logfile, __msg) {							\
	std::wstringstream _wsstream;								\
	_wsstream << L"[Error] " << __FILE__ << L"; line: "			\
		<< __LINE__ << L"; " << __msg << L'\n';					\
	Common::Debug::Logger::LogFn(__logfile, _wsstream.str());	\
	return FALSE;												\
}
#endif // ReturnFalse

#ifndef CheckReturn
#define CheckReturn(__logfile, __statement) {							\
	try {																\
		const BOOL _result = __statement;								\
		if (!_result) {													\
			std::wstringstream _wsstream;								\
			_wsstream << L"[Error] " << __FILE__						\
				<< L"; line: " << __LINE__ << L"; \n";					\
			Common::Debug::Logger::LogFn(__logfile, _wsstream.str());	\
			return FALSE;												\
		}																\
	}																	\
	catch (const std::exception& e) {									\
		std::wstringstream _wsstream;									\
		_wsstream << L"[Exception] " << __FILE__ << L"; line: "			\
			<< __LINE__ << L"; " << e.what() << L'\n';					\
		Common::Debug::Logger::LogFn(__logfile, _wsstream.str());		\
		return FALSE;													\
	}																	\
}
#endif // CheckReturn

#ifndef CheckHRESULT
#define CheckHRESULT(__logfile, __statement) {									\
	try {																		\
		const HRESULT _result = __statement;									\
		if (FAILED(_result)) {													\
			std::wstringstream _wsstream;										\
			_wsstream << L"[Error] " << __FILE__ << L"; line: " << __LINE__		\
				<< L"; HRESULT: 0x" << std::hex << _result << L'\n';			\
			Common::Debug::Logger::LogFn(__logfile, _wsstream.str());			\
			return FALSE;														\
		}																		\
	}																			\
	catch (const std::exception& e) {											\
		std::wstringstream _wsstream;											\
		_wsstream << L"[Exception] " << __FILE__ << L"; line: "					\
			<< __LINE__ << L"; " << e.what() << L'\n';							\
		Common::Debug::Logger::LogFn(__logfile, _wsstream.str());				\
		return FALSE;															\
	}																			\
}
#endif // CheckHRESULT

#ifndef CheckLastError
#define CheckLastError(__logfile, __statement)	{								\
	try {																		\
		const INT _result = __statement;										\
		if (_result == 0) {														\
			const INT _code = GetLastError();									\
			std::wstringstream _wsstream;										\
			_wsstream << L"[Error] " << __FILE__ << L"; line: " << __LINE__		\
				<< L"; HRESULT: 0x" << std::hex << _code << L'\n';				\
			Common::Debug::Logger::LogFn(__logfile, _wsstream.str());			\
			return FALSE;														\
		}																		\
	}																			\
	catch (const std::exception& e) {											\
		std::wstringstream _wsstream;											\
		_wsstream << L"[Exception] " << __FILE__ << L"; line: "					\
			<< __LINE__ << L"; " << e.what() << L'\n';							\
		Common::Debug::Logger::LogFn(__logfile, _wsstream.str());				\
		return FALSE;															\
	}																			\
}
#endif // CheckLastError

#ifndef NullCheck
#define NullCheck(__logfile, __object) {								\
	if (__object == nullptr) {											\
		Common::Debug::Logger::LogFn(__logfile, L"Null check failed");	\
		return FALSE;													\
	}																	\
}
#endif

#ifndef ConsoleLog
	#define ConsoleLog(__x, ...) {								\
		std::vector<std::string> _msgs = { __x, __VA_ARGS__ };	\
		std::stringstream _sstream;								\
		for (const auto& _msg : _msgs) _sstream << _msg;		\
		std::cout << _sstream.str() << std::endl;				\
	}
#endif

#ifndef ConsoleErr
	#define ConsoleErr(__x, ...) {								\
		std::vector<std::string> _msgs = { __x, __VA_ARGS__ };	\
		std::stringstream _sstream;								\
		for (const auto& _msg : _msgs) _sstream << _msg;		\
		std::err << _sstream.str() << std::endl;				\
	}
#endif

namespace Common::Debug {
	struct LogFile {
		HANDLE Handle = NULL;
		std::mutex Mutex;
	};

	class Logger {
	public:
		static BOOL Initialize(LogFile* const pLogFile, LPCWSTR filePaths);

		static void LogFn(LogFile* const pLogFile, const std::string& msg);
		static void LogFn(LogFile* const pLogFile, const std::wstring& msg);

 		static BOOL SetTextToWnd(LogFile* const pLogFile, HWND hWnd, LPCWSTR text);
		static BOOL AppendTextToWnd(LogFile* const pLogFile, HWND hWnd, LPCWSTR text);
	};
}