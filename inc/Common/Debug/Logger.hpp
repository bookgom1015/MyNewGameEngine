#pragma once

#include <exception>
#include <mutex>
#include <string>
#include <sstream>
#include <vector>
#include <Windows.h>

#ifndef Log
	#define Log(__x, ...) {										\
		std::vector<std::string> _msgs = { __x, __VA_ARGS__ };	\
		std::stringstream _sstream;								\
		_sstream << "[Log] ";									\
		for (const auto& _msg : _msgs) _sstream << _msg;		\
		Debug::Logger::LogFn(_sstream.str());					\
	}
#endif

#ifndef Logln
	#define Logln(__x, ...) {									\
		std::vector<std::string> _msgs = { __x, __VA_ARGS__ };	\
		std::stringstream _sstream;								\
		_sstream << "[Log] ";									\
		for (const auto& _msg : _msgs) _sstream << _msg;		\
		_sstream << '\n';										\
		Debug::Logger::LogFn(_sstream.str());					\
	}
#endif

#ifndef WLog
	#define WLog(__x, ...) {									\
		std::vector<std::wstring> _msgs = { __x, __VA_ARGS__ };	\
		std::wstringstream _wsstream;							\
		_wsstream << L"[Log] ";									\
		for (const auto& _msg : _msgs) _wsstream << _msg;		\
		Debug::Logger::LogFn(_wsstream.str());					\
	}
#endif

#ifndef WLogln
	#define WLogln(__x, ...) {									\
		std::vector<std::wstring> _msgs = { __x, __VA_ARGS__ };	\
		std::wstringstream _wsstream;							\
		_wsstream << L"[Log] ";									\
		for (const auto& _msg : _msgs) _wsstream << _msg;		\
		_wsstream << L'\n';										\
		Debug::Logger::LogFn(_wsstream.str());					\
	}
#endif

#ifndef ReturnFalse
	#define ReturnFalse(__msg) {							\
		std::wstringstream _wsstream;						\
		_wsstream << L"[Error] " << __FILE__ << L"; line: " \
			<< __LINE__ << L"; " << __msg << L'\n';			\
		Debug::Logger::LogFn(_wsstream.str());				\
		return FALSE;										\
	}
#endif

#ifndef CheckReturn
	#define CheckReturn(__statement) {								\
		try {														\
			const BOOL _result = __statement;						\
			if (!_result) {											\
				std::wstringstream _wsstream;						\
				_wsstream << L"[Error] " << __FILE__				\
					<< L"; line: " << __LINE__ << L"; \n";			\
				Debug::Logger::LogFn(_wsstream.str());				\
				return FALSE;										\
			}														\
		}															\
		catch (const std::exception& e) {							\
			std::wstringstream _wsstream;							\
			_wsstream << L"[Exception] " << __FILE__ << L"; line: "	\
				<< __LINE__ << L"; " << e.what() << L'\n';			\
			Debug::Logger::LogFn(_wsstream.str());					\
			return FALSE;											\
		}															\
	}
#endif

#ifndef CheckHRESULT
	#define CheckHRESULT(__statement) {													\
		try {																			\
			const HRESULT _result = __statement;										\
			if (FAILED(_result)) {														\
				std::wstringstream _wsstream;											\
				_wsstream << L"[Error] " << __FILE__ << L"; line: " << __LINE__			\
					<< L"; HRESULT: 0x" << std::hex << _result << L'\n';				\
				Debug::Logger::LogFn(_wsstream.str());									\
				return FALSE;															\
			}																			\
		}																				\
		catch (const std::exception& e) {												\
			std::wstringstream _wsstream;												\
			_wsstream << L"[Exception] " << __FILE__ << L"; line: "						\
				<< __LINE__ << L"; " << e.what() << L'\n';								\
			Debug::Logger::LogFn(_wsstream.str());										\
			return FALSE;																\
		}																	\
	}
#endif

#ifndef CheckLastError
	#define CheckLastError(__statement)	{										\
		try {																	\
			const INT _result = __statement;									\
			if (_result == 0) {													\
				const INT _code = GetLastError();								\
				std::wstringstream _wsstream;									\
				_wsstream << L"[Error] " << __FILE__ << L"; line: " << __LINE__	\
					<< L"; HRESULT: 0x" << std::hex << _code << L'\n';			\
				Debug::Logger::LogFn(_wsstream.str());							\
				return FALSE;													\
			}																	\
		}																		\
		catch (const std::exception& e) {										\
			std::wstringstream _wsstream;										\
			_wsstream << L"[Exception] " << __FILE__ << L"; line: "				\
				<< __LINE__ << L"; " << e.what() << L'\n';						\
			Debug::Logger::LogFn(_wsstream.str());								\
			return FALSE;														\
		}																		\
	}
#endif

namespace Debug {
	class Logger {
	public:
		static BOOL Initialize();

		static void LogFn(const std::string& msg);
		static void LogFn(const std::wstring& msg);

		static BOOL SetTextToWnd(HWND hWnd, LPCWSTR text);
		static BOOL AppendTextToWnd(HWND hWnd, LPCWSTR text);

	public:
		static HANDLE sLogFile;
		static std::mutex sLogFileMutex;
	};
}