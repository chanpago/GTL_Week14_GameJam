#include "pch.h"
#include "CrashHandler.h"

#include <dbghelp.h>
#include <Shlwapi.h>

#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "Shlwapi.lib")

wchar_t FCrashHandler::DumpDirectory[MAX_PATH] = L"CrashDumps";
bool FCrashHandler::bCrashInjection = false;

void FCrashHandler::Init()
{
    //  폴더가 없으면 생성
    CreateDirectoryW(DumpDirectory, nullptr);
    // 전역 Unhandled Exception 필터 등록
    ::SetUnhandledExceptionFilter(&FCrashHandler::UnhandledExceptionFilter);
}

LONG __stdcall FCrashHandler::UnhandledExceptionFilter(_EXCEPTION_POINTERS* ExceptionInfo)
{
    // 1. 시간 문자열 생성 (기존 로직 재사용)
    auto Now = std::chrono::system_clock::now();
    auto InTimeT = std::chrono::system_clock::to_time_t(Now);

    std::wstringstream TimeStream;
    struct tm TimeInfo;
    errno_t err = localtime_s(&TimeInfo, &InTimeT);

    if (err == 0)
    {
        TimeStream << std::put_time(&TimeInfo, L"%Y-%m-%d_%H-%M-%S");
    }
    else
    {
        TimeStream << L"UnknownTime";
    }
    std::wstring TimeString = TimeStream.str();

    // 2. 로그 파일 경로 생성 (.txt)
    std::wstring LogFileName = L"Mundi_CrashLog_" + TimeString + L".txt";
    std::wstring FullLogPath = std::wstring(DumpDirectory) + L"\\" + LogFileName;

    // ============================================================
    // [추가된 부분] 콜스택 추적 및 로그 저장
    // ============================================================
    HANDLE hProcess = GetCurrentProcess();
    HANDLE hThread = GetCurrentThread();

    // 심볼 초기화 (PDB 로드)
    SymInitialize(hProcess, NULL, TRUE);

    // 로그 파일 열기
    std::wofstream LogFile(FullLogPath);
    if (LogFile.is_open())
    {
        LogFile << L"=== Crash Detected ===" << std::endl;
        LogFile << L"Time: " << TimeString << std::endl;
        LogFile << L"Exception Code: 0x" << std::hex << ExceptionInfo->ExceptionRecord->ExceptionCode << std::dec << std::endl << std::endl;
        LogFile << L"=== Call Stack ===" << std::endl;

        // 스택 프레임 초기화
        STACKFRAME64 StackFrame;
        ZeroMemory(&StackFrame, sizeof(STACKFRAME64));
        DWORD MachineType = IMAGE_FILE_MACHINE_AMD64; // x64 기준

        CONTEXT Context = *ExceptionInfo->ContextRecord;

        StackFrame.AddrPC.Offset = Context.Rip;
        StackFrame.AddrPC.Mode = AddrModeFlat;
        StackFrame.AddrFrame.Offset = Context.Rbp;
        StackFrame.AddrFrame.Mode = AddrModeFlat;
        StackFrame.AddrStack.Offset = Context.Rsp;
        StackFrame.AddrStack.Mode = AddrModeFlat;

        // 스택 워킹 (루프)
        while (StackWalk64(MachineType, hProcess, hThread, &StackFrame, &Context, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL))
        {
            if (StackFrame.AddrPC.Offset == 0) break;

            // 1. 함수 이름 가져오기
            char SymbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
            PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)SymbolBuffer;
            pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            pSymbol->MaxNameLen = MAX_SYM_NAME;

            DWORD64 Displacement = 0;
            std::string FunctionName = "(Unknown Function)";

            if (SymFromAddr(hProcess, StackFrame.AddrPC.Offset, &Displacement, pSymbol))
            {
                FunctionName = pSymbol->Name;
            }

            // 2. 파일 및 라인 번호 가져오기
            IMAGEHLP_LINE64 Line;
            Line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
            DWORD DisplacementLine = 0;
            std::string FileName = "(Unknown File)";
            int LineNumber = 0;

            if (SymGetLineFromAddr64(hProcess, StackFrame.AddrPC.Offset, &DisplacementLine, &Line))
            {
                FileName = Line.FileName;
                LineNumber = Line.LineNumber;
            }

            // 3. 로그 파일에 기록
            // DbgHelp가 반환한 ANSI(std::string) 문자열을 Wide(std::wstring)로 변환
            FWideString WFunctionName = ACPToWide(FunctionName);
            
            // 파일 경로는 보기 좋게 정규화(NormalizePath)까지 적용하면 더 좋습니다
            FWideString WFileName = NormalizePath(ACPToWide(FileName));

            // 이제 wofstream에 wide string을 넘기면 한글이 깨지지 않고 기록됩니다.
            LogFile << WFunctionName << L" - " << WFileName << L" (" << LineNumber << L")" << std::endl;}

        LogFile.close();
    }
    
    // 심볼 정리
    SymCleanup(hProcess);

    // ============================================================
    // [기존 로직] 미니덤프 저장 (.dmp)
    // ============================================================
    std::wstring DumpFileName = L"Mundi_CrashDump_" + TimeString + L".dmp";
    std::wstring FullDumpPath = std::wstring(DumpDirectory) + L"\\" + DumpFileName;
    
    HANDLE hFile = CreateFileW(FullDumpPath.c_str(),
        GENERIC_WRITE, FILE_SHARE_READ, nullptr,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr
    );

    if (hFile != INVALID_HANDLE_VALUE)
    {
        _MINIDUMP_EXCEPTION_INFORMATION ExInfo;
        ExInfo.ThreadId = GetCurrentThreadId();
        ExInfo.ExceptionPointers = ExceptionInfo;
        ExInfo.ClientPointers = FALSE;
        MINIDUMP_TYPE DumpType = (MINIDUMP_TYPE)(MiniDumpNormal | MiniDumpWithDataSegs | MiniDumpWithHandleData);

        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
            hFile, DumpType, ExceptionInfo ? &ExInfo : nullptr, nullptr, nullptr);

        CloseHandle(hFile);
    }
    
    return EXCEPTION_EXECUTE_HANDLER;
}

void FCrashHandler::InjectCrash()
{
    bCrashInjection = true;
}

void FCrashHandler::Crash()
{
    if (!bCrashInjection) { return; }
    TArray<UObject*>& ObjectArray = GUObjectArray;
    if (ObjectArray.Num() == 0) return;

    bool bCrashInjected = false;
    while (!bCrashInjected)
    {
        int32 RandomIndex = rand() % ObjectArray.Num();
        UObject* Victim = ObjectArray[RandomIndex];

        void** VTablePtr = reinterpret_cast<void**>(Victim);
        if (*VTablePtr == reinterpret_cast<void*>(0xDEADBEEFDEADBEEF))
        {
            continue;
        }

        bCrashInjected = true;
        *VTablePtr = reinterpret_cast<void*>(0xDEADBEEFDEADBEEF);
    }
}
