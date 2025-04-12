#pragma once

#include <string>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <Windows.h>

namespace Common::Util {
	class StringUtil {
	public:
		static std::string WStringToString(const std::wstring& wstr);
        static std::wstring StringToWString(const std::string& str);
	};
}