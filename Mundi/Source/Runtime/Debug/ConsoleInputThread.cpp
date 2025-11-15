#include "pch.h"
#include "Console.h"
#include <iostream>
#include <thread>

void ConsoleInputThread()
{
	std::wstring input;
	FConsole& Console = FConsole::GetInstance();

	while (true)
	{
		std::wcout << L" Console > ";
		std::getline(std::wcin, input);

		if (!input.empty())
		{
			std::wcout << input;
			Console.Exectue(input);
		}
	}
}

void StartConsoleThread()
{
	std::thread consoleThread(ConsoleInputThread);
	consoleThread.detach();
}