#include "StdInc.h"
#include "ProgramArguments.h"
#include "RTMain.h"

#include "Game.h"

#include <stdio.h>

#include <iostream>

#include <CdImageDevice.h>
#include <Streaming.h>
#include <vfs/Manager.h>

#include <Console.h>
#include <Console.CommandHelpers.h>

#include <Streaming.tests.h>
#include <WorldMath.h>

namespace krt
{
int unknown()
{
	vfs::StreamPtr stream = vfs::OpenRead("C:\\Windows\\system32\\License.rtf");
	auto data             = stream->ReadToEnd();

	assert(false && "hey!");

	return 0;
}

int Main::Run(const ProgramArguments& arguments)
{
    Game TheGame;

	ConsoleCommand cmd("a", [] (int a, int b, int c)
	{
		printf("%d %d %d\n", a, b, c);
	});

	ConsoleCommand cmd2("a", [] (int a, int b, const std::string& c)
	{
		printf("%d %d %s\n", a, b, c.c_str());
	});

	console::ExecuteSingleCommand("a 5 4 1000");
	console::AddToBuffer("a 1 2 3; a 1 2 \"hi world\"\r\na 1 2");
	console::ExecuteBuffer();

    // ))))))))))))))
    math::Tests();
    streaming::FaultTest1();
    streaming::Test3();

	unknown();
	unknown();
	unknown();

	if (arguments.Count() >= 2)
	{
		printf("hi! %s\n", arguments[2].c_str());
	}
	else
	{
		printf("meow\n");
	}

	return 0;
}
}