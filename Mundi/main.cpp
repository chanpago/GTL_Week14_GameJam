#include "pch.h"
#include "EditorEngine.h"
#include "Source/Runtime/Debug/CrashHandler.h"
#include "Source/Runtime/Debug/CrashCommand.h"
#include "Source/Runtime/Debug/Console.h" 
// C runtime for freopen_s
#include <cstdio>


#if defined(_MSC_VER) && defined(_DEBUG)
#   define _CRTDBG_MAP_ALLOC
#   include <cstdlib>
#   include <crtdbg.h>
#endif

extern void StartConsoleThread();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
    _CrtSetBreakAlloc(0);
#endif

    FCrashHandler::Init();  

    // Attach to parent console (if launched from cmd) or create a new console.
    //if (!AttachConsole(ATTACH_PARENT_PROCESS))
    {
        AllocConsole();
    }
    FILE* fDummy;
    freopen_s(&fDummy, "CONIN$", "r", stdin);
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);

    FConsole& Console = FConsole::GetInstance();
    // Crash 명령어 등록
    Console.RegisterCommand(L"Crash", []() {
        CCrashCommand CrashCmd;
        CrashCmd.CauseCrash();
    }); 

    StartConsoleThread();

    if (!GEngine.Startup(hInstance))
        return -1;

    try
    {
        GEngine.MainLoop();
        GEngine.Shutdown();
    }
    catch (const std::exception& e)
    {
        throw;
    }

    return 0;
}
