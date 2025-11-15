#pragma once
#include <functional>
#include "Object.h"
#include "UEContainer.h"

class FConsole : public UObject
{
public:
	DECLARE_CLASS(FConsole, UObject)

	using FCommand = std::function<void()>;

	static FConsole& GetInstance();

	void RegisterCommand(const std::wstring& Name, FCommand Cmd);
	void Exectue(const std::wstring& Name); 

private: 
	TMap<std::wstring, FCommand> Commands;

};