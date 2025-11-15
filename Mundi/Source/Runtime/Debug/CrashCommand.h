#pragma once
#include <UEContainer.h>
  
class CCrashCommand
{
public:
    CCrashCommand() = default;
    ~CCrashCommand() = default;
        
    void CauseCrash(); 
};