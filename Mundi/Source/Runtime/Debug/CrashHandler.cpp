#include "pch.h"
#include "CrashHandler.h"

#include <dbghelp.h>
#include <Shlwapi.h>

#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "Shlwapi.lib")

wchar_t FCrashHandler::DumpDirectory[MAX_PATH] = L"CrashDumps";

void FCrashHandler::Init(const wchar_t* InDumpDir)
{
    // 덤프 저장 폴더 설정
    wcsncpy_s(DumpDirectory, InDumpDir, _TRUNCATE);

    //  폴더가 없으면 생성
    CreateDirectoryW(DumpDirectory, nullptr);

    // 전역 Unhandled Exception 필터 등록
    ::SetUnhandledExceptionFilter(&FCrashHandler::UnhandledExceptionFilter);

}

LONG __stdcall FCrashHandler::UnhandledExceptionFilter(_EXCEPTION_POINTERS* ExceptionInfo)
{
    // 덤프 파일 경로 생성
    SYSTEMTIME SystemTime;

    GetLocalTime(&SystemTime);

    wchar_t FileName[MAX_PATH];
    swprintf_s(
        FileName,
        L"%s\\Crash_%02d%02d_%02d%02d%02d.dmp",
        DumpDirectory,
        SystemTime.wMonth, SystemTime.wDay,
        SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond
    );

    HANDLE hFile = CreateFileW(
        FileName,
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hFile != INVALID_HANDLE_VALUE)
    {
        MINIDUMP_EXCEPTION_INFORMATION DumpInfo{};
        DumpInfo.ThreadId = GetCurrentThreadId();
        DumpInfo.ExceptionPointers = ExceptionInfo;
        DumpInfo.ClientPointers = FALSE;

        // 덤프 타입은 필요에 따라 조정 가능
        //MiniDumpWithDataSegs
        //MiniDumpWithHandleData
        //MiniDumpWithUnloadedModules
        //MiniDumpWithThreadInfo
        MINIDUMP_TYPE DumpType = static_cast<MINIDUMP_TYPE>(
            MiniDumpWithIndirectlyReferencedMemory |
            MiniDumpScanMemory);

        MiniDumpWriteDump(
            GetCurrentProcess(),
            GetCurrentProcessId(),
            hFile,
            DumpType,
            &DumpInfo,
            nullptr, nullptr
        );

        CloseHandle(hFile);
    }

    // 예외처리했고, 프로세스 종료하라는 의미 
    return EXCEPTION_EXECUTE_HANDLER;
}
