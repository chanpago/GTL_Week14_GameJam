#include "pch.h"
#include "EditorEngine.h"
#include "Source/Runtime/Debug/CrashHandler.h"

#if defined(_MSC_VER) && defined(_DEBUG)
#   define _CRTDBG_MAP_ALLOC
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
