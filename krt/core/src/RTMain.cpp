#include "StdInc.h"
#include "ProgramArguments.h"
#include "RTMain.h"

#include "Game.h"

namespace krt
{
int Main::Run(const ProgramArguments& arguments)
{
	Game TheGame;

	TheGame.Run();

	return 0;
}
}