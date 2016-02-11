#include "StdInc.h"
#include <windows.h>

#include <RTMain.h>
#include <ProgramArguments.h>

int main(int argc, char** argv)
{
	krt::ProgramArguments arguments(argc, argv);
	krt::Main main;

	return main.Run(arguments);
}