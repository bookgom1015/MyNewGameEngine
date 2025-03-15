#pragma once

#include <string>

#include <Windows.h>

namespace Common::Foundation {
	class StringUtil {
	public:
		static std::string WStringToString(const std::wstring& wstr);
        static std::wstring StringToWString(const std::string& str);
	};
}