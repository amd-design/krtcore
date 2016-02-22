#include "StdInc.h"
#include "ProgramArguments.h"
#include "RTMain.h"

#include "Game.h"

#include "WorldMath.h"

namespace krt
{
int Main::Run(const ProgramArguments& arguments)
{
    // Run test code, meow.
    // We should do that to self assert our correctness!
    krt::math::Tests();

	// parse argument list and handle console commands
	sys::InitializeConsole();

	// the argument list to pass after the game initialized
	std::vector<ProgramArguments> argumentBits;
	std::vector<std::pair<std::string, std::string>> setList;

	{
		for (size_t i : irange(arguments.Count()))
		{
			const std::string& argument = arguments[i];

			// if this is a command-like
			if (argument.length() > 1 && argument[0] == '+')
			{
				// if this is a set command, queue it for the early execution buffer
				if (argument == "+set" && (i + 2) < arguments.Count())
				{
					setList.push_back({ arguments[i + 1], arguments[i + 2] });
				}
				else
				{
					// capture bits up to the next +
					std::vector<std::string> thisBits;
					thisBits.push_back(argument.substr(1));

					for (size_t j : irange<size_t>(i + 1, arguments.Count()))
					{
						const std::string& bit = arguments[j];

						if (bit.length() > 0 && bit[0] == '+')
						{
							break;
						}
						
						thisBits.push_back(bit);
					}

					argumentBits.push_back(ProgramArguments{ thisBits });
				}
			}
		}
	}

	{
		// create and initialize the game
		Game TheGame(setList);

		// run argument bits captures
		for (const auto& bit : argumentBits)
		{
			console::ExecuteSingleCommand(bit);
		}

		// run the game
		TheGame.Run();
	}

	return 0;
}
}