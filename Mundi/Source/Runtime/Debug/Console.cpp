#include "pch.h"
#include "Console.h"
IMPLEMENT_CLASS(FConsole)


FConsole& FConsole::GetInstance()
{
    static FConsole* Instance = nullptr;
    if (Instance == nullptr)
    {
        Instance = NewObject<FConsole>();
    }
    return *Instance;
}

void FConsole::RegisterCommand(const std::wstring& Name, FCommand Cmd)
{
    Commands[Name] = std::move(Cmd); 
}

void FConsole::Exectue(const std::wstring& Name)
{
    auto it = Commands.find(Name);
    if (it != Commands.end())
    {
        it->second();
    } 
}
 