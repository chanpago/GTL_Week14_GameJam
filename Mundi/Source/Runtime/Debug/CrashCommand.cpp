#include "pch.h"
#include <Windows.h>
#include "CrashCommand.h"
  
void CCrashCommand::CauseCrash()
{
    DWORD exceptionCode = 0xE0000001; // 사용자 정의 예외 코드
    RaiseException(
        exceptionCode,
        EXCEPTION_NONCONTINUABLE, // 계속 실행 불가 예외
        0,
        nullptr

    );
     

}
 