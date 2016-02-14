#pragma once

#include <ProgramArguments.h>

namespace krt
{
class Console
{
public:
	static void ExecuteSingleCommand(const std::string& command);

	static void ExecuteSingleCommand(const ProgramArguments& arguments);

	static ProgramArguments Tokenize(const std::string& line);

	static void AddToBuffer(const std::string& text);

	static void ExecuteBuffer();
};
}