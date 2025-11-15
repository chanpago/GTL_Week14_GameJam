#pragma once
#include <Windows.h>

class FCrashHandler
{
public:
	static void Init(const wchar_t* InDumpDir = L"CrashDumps");
	static LONG WINAPI UnhandledExceptionFilter(_In_ struct _EXCEPTION_POINTERS* ExceptionInfo);
		
private:
	static wchar_t DumpDirectory[MAX_PATH];
};