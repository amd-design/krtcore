#include <StdInc.h>
#include <Console.Base.h>

#include <Console.VariableHelpers.h>

#include <stdarg.h>

namespace krt
{
namespace console
{
static std::vector<void(*)(const char*)> g_printListeners;
static int g_useDeveloper;

void Printf(const char* format, ...)
{
	// this'll only initialize the buffer if needed
	static thread_local std::vector<char> buffer(32768);

	va_list ap;
	
	va_start(ap, format);
	vsnprintf(&buffer[0], buffer.size(), format, ap);
	va_end(ap);

	// always zero-terminate
	buffer[buffer.size() - 1] = '\0';

	// print to all interested listeners
	for (auto& listener : g_printListeners)
	{
		listener(buffer.data());
	}
}

void DPrintf(const char* format, ...)
{
	if (g_useDeveloper > 0)
	{
		// this'll only initialize the buffer if needed
		static thread_local std::vector<char> buffer(32768);

		va_list ap;

		va_start(ap, format);
		vsnprintf(&buffer[0], buffer.size(), format, ap);
		va_end(ap);

		// always zero-terminate
		buffer[buffer.size() - 1] = '\0';

		// do print
		Printf("%s", buffer.data());
	}
}

void PrintWarning(const char* format, ...)
{
	// this'll only initialize the buffer if needed
	static thread_local std::vector<char> buffer(32768);

	va_list ap;

	va_start(ap, format);
	vsnprintf(&buffer[0], buffer.size(), format, ap);
	va_end(ap);

	// always zero-terminate
	buffer[buffer.size() - 1] = '\0';

	// print the string directly
	Printf("^3Warning: %s^7", buffer.data());
}

void PrintError(const char* format, ...)
{
	// this'll only initialize the buffer if needed
	static thread_local std::vector<char> buffer(32768);

	va_list ap;

	va_start(ap, format);
	vsnprintf(&buffer[0], buffer.size(), format, ap);
	va_end(ap);

	// always zero-terminate
	buffer[buffer.size() - 1] = '\0';

	// print the string directly
	Printf("^1Error: %s^7", buffer.data());
}

void AddPrintListener(void(*function)(const char*))
{
	g_printListeners.push_back(function);
}

static ConVar<int> developerVariable("developer", ConVar_Archive, 0, &g_useDeveloper);
}
}