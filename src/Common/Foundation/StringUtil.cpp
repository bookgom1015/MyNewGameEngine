#include "Common/Foundation/StringUtil.hpp"

using namespace Common::Foundation;

std::string StringUtil::WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    UINT size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string str(size - 1, 0); // To remove '0'
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], size, nullptr, nullptr);
    return str;
}

std::wstring StringUtil::StringToWString(const std::string& str) {
    if (str.empty()) return L"";
    UINT size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    std::wstring wstr(size - 1, 0); // To remove '0'
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size);
    return wstr;
}