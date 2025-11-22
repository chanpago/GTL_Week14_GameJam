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
// 1. 시간 문자열 생성
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

    // 2. 로그 파일 경로 생성
    std::wstring LogFileName = L"Mundi_CrashLog_" + TimeString + L".txt";
    std::wstring FullLogPath = std::wstring(DumpDirectory) + L"\\" + LogFileName;

    // ============================================================
    //  콜스택 추적 및 로그 저장 (하이브리드 방식 적용)
    // ============================================================
    HANDLE hProcess = GetCurrentProcess();
    HANDLE hThread = GetCurrentThread();

    // 우리가 만든 하이브리드 경로 가져오기
    std::string SearchPath = GetSymbolSearchPath();

    // 옵션 설정
    // SYMOPT_DEBUG: 디버그 출력 켜기 (문제 발생 시 Output 창 확인용)
    // SYMOPT_LOAD_LINES: 소스 코드 라인 번호 로드
    // SYMOPT_UNDNAME: 함수 이름 보기 좋게 정렬
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_DEBUG);

    // 심볼 초기화
    SymInitialize(hProcess, SearchPath.c_str(), TRUE);

    // 로그 파일 열기
    std::wofstream LogFile(FullLogPath);
    if (LogFile.is_open())
    {
        LogFile << L"=== Crash Detected ===" << std::endl;
        LogFile << L"Time: " << TimeString << std::endl;
        LogFile << L"Exception Code: 0x" << std::hex << ExceptionInfo->ExceptionRecord->ExceptionCode << std::dec << std::endl;
        LogFile << L"Symbol Path Used: " << ACPToWide(SearchPath) << std::endl << std::endl;
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

        // 스택 워킹
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
            FWideString WFunctionName = ACPToWide(FunctionName);
            FWideString WFileName = NormalizePath(ACPToWide(FileName));

            LogFile << WFunctionName << L" - " << WFileName << L" (" << LineNumber << L")" << std::endl;
        }

        LogFile.close();
    }
    
    // 심볼 정리
    SymCleanup(hProcess);

    // ============================================================
    // 미니덤프 저장
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
    
    return EXCEPTION_EXECUTE_HANDLER;}

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

FString FCrashHandler::GetSymbolSearchPath()
{
    // 로컬 PDB 우선
    char ExePath[MAX_PATH];
    GetModuleFileNameA(NULL, ExePath, MAX_PATH);
    std::string LocalDir = ExePath;
    size_t LastSlash = LocalDir.find_last_of("\\/");
    if (LastSlash != std::string::npos)
    {
        LocalDir = LocalDir.substr(0, LastSlash);
    }

    // 임시 폴더(SymbolCache) 경로
    char TempPath[MAX_PATH];
    GetTempPathA(MAX_PATH, TempPath);
    std::string CacheDir = std::string(TempPath) + "SymbolCache";
    CreateDirectoryA(CacheDir.c_str(), NULL);

    // 심볼 서버 경로
    std::string ServerPath = R"(\\172.21.11.109\SymbolStore)";

    // 경로 합치기 (우선순위 순서대로 세미콜론 ; 으로 구분)
    std::stringstream ss;
    ss << LocalDir << ";"; // 1순위: 로컬
    ss << "srv*" << CacheDir << "*" << ServerPath; // 2순위: 없으면 서버에서 다운받아 캐시에 저장

    return ss.str();
}
