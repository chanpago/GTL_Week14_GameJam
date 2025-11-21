#pragma once
#include <Windows.h>

class FCrashHandler
{
public:
	static void Init();
	static LONG WINAPI UnhandledExceptionFilter(_In_ struct _EXCEPTION_POINTERS* ExceptionInfo);
	static void InjectCrash();
	static void Crash();
		
private:
	static bool bCrashInjection;
	static wchar_t DumpDirectory[MAX_PATH];
};
