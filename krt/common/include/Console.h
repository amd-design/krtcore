#pragma once

#include <Console.Commands.h>
#include <ProgramArguments.h>

namespace krt
{
namespace console
{
class Context
{
public:
	Context();

	Context(Context* fallbackContext);

	void ExecuteSingleCommand(const std::string& command);

	void ExecuteSingleCommand(const ProgramArguments& arguments);

	void AddToBuffer(const std::string& text);

	void ExecuteBuffer();

	inline ConsoleCommandManager* GetCommandManager()
	{
		return m_commandManager.get();
	}

	inline Context* GetFallbackContext()
	{
		return m_fallbackContext;
	}

private:
	Context* m_fallbackContext;

	std::unique_ptr<ConsoleCommandManager> m_commandManager;

	std::string m_commandBuffer;

	std::mutex m_commandBufferMutex;
};

Context* GetDefaultContext();

void ExecuteSingleCommand(const std::string& command);

void ExecuteSingleCommand(const ProgramArguments& arguments);

ProgramArguments Tokenize(const std::string& line);

void AddToBuffer(const std::string& text);

void ExecuteBuffer();
}
}