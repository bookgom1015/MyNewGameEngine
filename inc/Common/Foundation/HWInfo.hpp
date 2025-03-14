#pragma once

#include <string>
#include <sstream>

#include <Windows.h>

namespace Common::Debug {
	struct LogFile;
}

namespace Common::Foundation {
	struct Processor {
		std::wstring Name;

		BOOL SupportMMX = FALSE;
		BOOL SupportSSE = FALSE;
		BOOL SupportSSE2 = FALSE;
		BOOL SupportSSE3 = FALSE;
		BOOL SupportSSSE3 = FALSE;
		BOOL SupportSSE4_1 = FALSE;
		BOOL SupportSSE4_2 = FALSE;
		BOOL SupportAVX = FALSE;
		BOOL SupportAVX2 = FALSE;
		BOOL SupportAVX512F = FALSE;  // AVX-512 Foundation
		BOOL SupportAVX512DQ = FALSE; // AVX-512 Doubleword & Quadword
		BOOL SupportAVX512BW = FALSE; // AVX-512 Byte & Word

		UINT64 Physical = 0;
		UINT64 Logical = 0;

		UINT64 TotalPhysicalMemory = 0;
		UINT64 AvailablePhysicalMemory = 0;
		UINT64 TotalVirtualMemory = 0;
		UINT64 AvailableVirtualMemory = 0;
	};

	class HWInfo {
	public:
		static BOOL ProcessorInfo(Common::Debug::LogFile* const pLogFile, Processor& info);

	public:
		static BOOL GetProcessorName(Common::Debug::LogFile* const pLogFile, Processor& info);
		static BOOL GetInstructionSupport(Common::Debug::LogFile* const pLogFile, Processor& info);
		static BOOL GetCoreInfo(Common::Debug::LogFile* const pLogFile, Processor& info);
		static BOOL GetSystemMemoryInfo(Common::Debug::LogFile* const pLogFile, Processor& info);
	};
}