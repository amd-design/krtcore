#include "StdInc.h"
#include "RTMain.h"
#include "ProgramArguments.h"

#include <stdio.h>

namespace krt
{
int unknown()
{
	assert(false && "hey!");

	return 0;
}

int Main::Run(const ProgramArguments& arguments)
{
	unknown();
	unknown();
	unknown();

	printf("hi! %s\n", arguments[2].c_str());

	return 0;
}
}