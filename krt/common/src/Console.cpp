#include <StdInc.h>
#include <Console.h>
#include <Console.Commands.h>

#include <sstream>

namespace krt
{
void Console::ExecuteSingleCommand(const std::string& command)
{
	ProgramArguments arguments = Tokenize(command);

	ExecuteSingleCommand(arguments);
}

void Console::ExecuteSingleCommand(const ProgramArguments& arguments)
{
	// early out if no command nor arguments were passed
	if (arguments.Count() == 0)
	{
		return;
	}

	// make a copy of the arguments to shift off the command name
	ProgramArguments localArgs(arguments);
	std::string command = localArgs.Shift();

	// run the command through the command manager
	ConsoleCommandManager::GetInstance()->Invoke(command, localArgs);
}

static std::string g_commandBuffer;
static std::mutex g_commandBufferMutex;

void Console::AddToBuffer(const std::string& text)
{
	std::lock_guard<std::mutex> guard(g_commandBufferMutex);
	g_commandBuffer += text;
}

void Console::ExecuteBuffer()
{
	std::vector<std::string> toExecute;

	{
		std::lock_guard<std::mutex> guard(g_commandBufferMutex);

		while (g_commandBuffer.length() > 0)
		{
			// parse the command up to the first occurence of a newline/semicolon
			int i = 0;
			bool inQuote = false;

			size_t cbufLength = g_commandBuffer.length();

			for (i = 0; i < cbufLength; i++)
			{
				if (g_commandBuffer[i] == '"')
				{
					inQuote = !inQuote;
				}

				// break if a semicolon
				if (!inQuote && g_commandBuffer[i] == ';')
				{
					break;
				}

				// or a newline
				if (g_commandBuffer[i] == '\r' || g_commandBuffer[i] == '\n')
				{
					break;
				}
			}

			std::string command = g_commandBuffer.substr(0, i);

			if (cbufLength > i)
			{
				g_commandBuffer = g_commandBuffer.substr(i + 1);
			}
			else
			{
				g_commandBuffer.clear();
			}

			// and add the command for execution when the mutex is unlocked
			toExecute.push_back(command);
		}
	}

	for (const std::string& command : toExecute)
	{
		ExecuteSingleCommand(command);
	}
}

ProgramArguments Console::Tokenize(const std::string& line)
{
	int i = 0;
	int j = 0;
	std::vector<std::string> args;

	size_t lineLength = line.length();

	// outer loop
	while (true)
	{
		// inner loop to skip whitespace
		while (true)
		{
			// skip whitespace and control characters
			while (i < lineLength && line[i] <= ' ') // ASCII only?
			{
				i++;
			}

			// return if needed
			if (i >= lineLength)
			{
				return ProgramArguments{ args };
			}

			// allegedly fixes issues with parsing
			if (i == 0)
			{
				break;
			}

			// skip comments
			if ((line[i] == '/' && line[i + 1] == '/') || line[i] == '#') // full line is a comment
			{
				return ProgramArguments{ args };
			}

			// /* comments
			if (line[i] == '/' && line[i + 1] == '*')
			{
				while (i < (lineLength + 1) && (line[i] != '*' && line[i + 1] != '/'))
				{
					i++;
				}

				if (i >= lineLength)
				{
					return ProgramArguments{ args };
				}

				i += 2;
			}
			else
			{
				break;
			}
		}

		// there's a new argument on the block
		std::stringstream arg;

		// quoted strings
		if (line[i] == '"')
		{
			bool inEscape = false;

			while (true)
			{
				i++;

				if (i >= lineLength)
				{
					break;
				}

				if (line[i] == '"' && !inEscape)
				{
					break;
				}

				if (line[i] == '\\')
				{
					inEscape = true;
				}
				else
				{
					arg << line[i];
					inEscape = false;
				}
			}

			i++;

			args.push_back(arg.str());
			j++;

			if (i >= lineLength)
			{
				return ProgramArguments{ args };
			}

			continue;
		}

		// non-quoted strings
		while (i < lineLength && line[i] > ' ')
		{
			if (line[i] == '"')
			{
				break;
			}

			if (i < (lineLength - 1))
			{
				if ((line[i] == '/' && line[i + 1] == '/') || line[i] == '#')
				{
					break;
				}

				if (line[i] == '/' && line[i + 1] == '*')
				{
					break;
				}
			}

			arg << line[i];

			i++;
		}

		args.push_back(arg.str());
		j++;

		if (i >= lineLength)
		{
			return ProgramArguments{ args };
		}
	}
}
}