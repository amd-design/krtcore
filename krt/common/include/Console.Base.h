#pragma once

namespace krt
{
namespace console
{
void Printf(const char* format, ...);

void DPrintf(const char* format, ...);

void PrintWarning(const char* format, ...);

void PrintError(const char* format, ...);

// NOT thread-safe!
void AddPrintListener(void (*function)(const char*));
}

namespace sys
{
void InitializeConsole();

const char* GetConsoleInput();
}
}