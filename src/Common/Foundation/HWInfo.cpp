#include "Common/Foundation/HWInfo.hpp"
#include "Common/Debug/Logger.hpp"

#include <intrin.h>

using namespace Common::Foundation;

namespace {
    const WCHAR* const STR_FAIL_GET_PROC_INFO = L"Failed to retrieve processor information.";
    const WCHAR* const STR_FAIL_GET_BUFF_SIZE = L"Failed to get required buffer size.";
    const WCHAR* const STR_FAIL_GET_MEM_STAT = L"Failed to get memory status";
}

BOOL HWInfo::ProcessorInfo(Common::Debug::LogFile* const pLogFile, Processor& info) {
    CheckReturn(pLogFile, GetProcessorName(pLogFile, info));
    CheckReturn(pLogFile, GetInstructionSupport(pLogFile, info));
    CheckReturn(pLogFile, GetCoreInfo(pLogFile, info));
    CheckReturn(pLogFile, GetSystemMemoryInfo(pLogFile, info));

	return TRUE;
}

BOOL HWInfo::GetProcessorName(Common::Debug::LogFile* const pLogFile, Processor& info) {
    INT CPUInfo[4] = { -1 };
    CHAR CPUBrandString[0x40] = { 0 };

    // Processor's name
    __cpuid(CPUInfo, 0x80000002);
    memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));

    __cpuid(CPUInfo, 0x80000003);
    memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
    
    __cpuid(CPUInfo, 0x80000004);
    memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));

    std::wstringstream wsstream;
    wsstream << CPUBrandString;

    info.Name = wsstream.str();

    return TRUE;
}

BOOL HWInfo::GetInstructionSupport(Common::Debug::LogFile* const pLogFile, Processor& info) {
    INT CPUInfo[4] = { 0 };

    // Call default CPUID (EAX=1)
    __cpuid(CPUInfo, 1);

    // Check if SSE, SSE2 are supported in EDX register
    info.SupportMMX  = (CPUInfo[3] & (1 << 23)) != 0; // EDX 23th bit (MMX)
    info.SupportSSE  = (CPUInfo[3] & (1 << 25)) != 0; // EDX 25th bit (SSE)
    info.SupportSSE2 = (CPUInfo[3] & (1 << 26)) != 0; // EDX 26th bit (SSE2)

    // Check if SSE3, AVX, etc. are supported in ECX register
    info.SupportSSE3   = (CPUInfo[2] & (1 << 0))  != 0; // ECX 0th  bit (SSE3)
    info.SupportSSSE3  = (CPUInfo[2] & (1 << 9))  != 0; // ECX 9th  bit (SSSE3)
    info.SupportSSE4_1 = (CPUInfo[2] & (1 << 19)) != 0; // ECX 19th bit (SSE4.1)
    info.SupportSSE4_2 = (CPUInfo[2] & (1 << 20)) != 0; // ECX 20th bit (SSE4.2)
    info.SupportAVX    = (CPUInfo[2] & (1 << 28)) != 0; // ECX 28th bit (AVX)

    // Call CPUID (EAX=7, ECX=0)
    __cpuidex(CPUInfo, 7, 0);

    info.SupportAVX2     = (CPUInfo[1] & (1 << 5))  != 0; // EBX 5th  bit (AVX2)
    info.SupportAVX512F  = (CPUInfo[1] & (1 << 16)) != 0; // EBX 16th bit (AVX512F)
    info.SupportAVX512DQ = (CPUInfo[1] & (1 << 17)) != 0; // EBX 17th bit (AVX512DQ)
    info.SupportAVX512BW = (CPUInfo[1] & (1 << 30)) != 0; // EBX 30th bit (AVX512BW)

    return TRUE;
}

BOOL HWInfo::GetCoreInfo(Common::Debug::LogFile* const pLogFile, Processor& info) {
    DWORD length = 0;
    GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &length);

    if (length == 0) ReturnFalse(pLogFile, STR_FAIL_GET_BUFF_SIZE);

    std::vector<uint8_t> buffer(length);
    if (!GetLogicalProcessorInformationEx(RelationProcessorCore, reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(buffer.data()), &length)) 
        ReturnFalse(pLogFile, STR_FAIL_GET_PROC_INFO);

    info.Physical = 0;
    info.Logical = 0;

    for (size_t offset = 0; offset < length;) {
        auto* const infoEX = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(buffer.data() + offset);

        if (infoEX->Relationship == RelationProcessorCore) {
            ++info.Physical;
            info.Logical += __popcnt64(infoEX->Processor.GroupMask->Mask);
        }

        offset += infoEX->Size;
    }

    return TRUE;
}

BOOL HWInfo::GetSystemMemoryInfo(Common::Debug::LogFile* const pLogFile, Processor& info) {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);

    if (!GlobalMemoryStatusEx(&memInfo)) ReturnFalse(pLogFile, STR_FAIL_GET_MEM_STAT);

    UINT64 denom = 1024 * 1024;
    info.TotalPhysicalMemory = memInfo.ullTotalPhys / denom;
    info.AvailablePhysicalMemory = memInfo.ullAvailPhys / denom;
    info.TotalVirtualMemory = memInfo.ullTotalVirtual / denom;
    info.AvailableVirtualMemory = memInfo.ullAvailVirtual / denom;

    return TRUE;
}