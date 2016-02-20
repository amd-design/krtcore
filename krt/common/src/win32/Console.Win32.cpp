#include <StdInc.h>
#include <Console.Base.h>

#include <windows.h>

#include <conio.h>
#include <fcntl.h>
#include <io.h>

#include <thread>
#include <concurrent_queue.h>

namespace krt
{
namespace sys
{
static const int g_colors[] = {
	FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY, // black, but actually bright white
	FOREGROUND_RED | FOREGROUND_INTENSITY, // red
	FOREGROUND_GREEN | FOREGROUND_INTENSITY, // green
	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY, // yellow
	FOREGROUND_BLUE | FOREGROUND_INTENSITY, // blue
	FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY, // cyan
	FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY, // magenta
	FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED, // neutral
	FOREGROUND_RED, // dark red
	FOREGROUND_BLUE, // dark blue
};

static int MapColorCode(char color)
{
	return g_colors[color - '0'];
}

static void PrintConsole(const char* string)
{
	for (const char* p = string; *p; p++)
	{
		if (*p == '^' && isdigit(p[1]))
		{
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), MapColorCode(p[1]));

			++p;
		}
		else
		{
			DWORD dummy;
			WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), p, 1, &dummy, nullptr);
		}
	}
}

void InitializeConsole()
{
	console::AddPrintListener(PrintConsole);
}

// we're assuming we're using VC
concurrency::concurrent_queue<std::string> commandQueue;

const char* GetConsoleInput()
{
	static bool inputInited;

	if (!inputInited)
	{
		std::thread([] ()
		{
			static char	text[2048];
			static int		len;
			int		c;

			DWORD mode = 0;
			GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode);
			SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode & (~ENABLE_ECHO_INPUT) & (~ENABLE_LINE_INPUT));

			_setmode(_fileno(stdin), _O_BINARY);

			// read a line out
			while ((c = getchar()) != EOF)
			{
				//c = _getch();
				printf("%c", c);
				if (c == '\r')
				{
					text[len] = '\n';
					text[len + 1] = 0;
					printf("%c", '\n');
					len = 0;

					commandQueue.push(text);
				}
				if (c == 8)
				{
					if (len)
					{
						printf(" %c", c);
						len--;
						text[len] = 0;
					}
					continue;
				}
				text[len] = c;
				len++;
				text[len] = 0;
				if (len == (sizeof(text) - 1))
					len = 0;
			}
		}).detach();

		inputInited = true;
	}

	if (commandQueue.empty())
	{
		return nullptr;
	}

	std::string entry;
	if (commandQueue.try_pop(entry))
	{
		static std::string entryText;
		entryText = entry;

		return entryText.c_str();
	}

	return nullptr;
}
}
}