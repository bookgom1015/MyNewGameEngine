#pragma once

#include <string>

#include <Windows.h>

namespace Common::Foundation::Util {
	class StringUtil {
	public:
		static std::string WStringToString(const std::wstring& wstr);
        static std::wstring StringToWString(const std::string& str);
	};
}