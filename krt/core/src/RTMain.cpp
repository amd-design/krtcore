#include "StdInc.h"
#include "RTMain.h"
#include "ProgramArguments.h"

#include <stdio.h>

#include <vfs/Manager.h>

namespace krt
{
int unknown()
{
	vfs::StreamPtr stream = vfs::OpenRead("C:\\Windows\\system32\\License.rtf");
	auto data = stream->ReadToEnd();

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