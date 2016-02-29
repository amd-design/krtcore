#pragma once

#include <Console.Commands.h>
#include <ProgramArguments.h>

namespace krt
{
class ConsoleManagersBase
{
public:
	virtual ~ConsoleManagersBase() = default;
};

class ConsoleVariableManager;
class BindingManager;

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

	void SaveConfigurationIfNeeded(const std::string& path);

	void SetVariableModifiedFlags(int flags);

	ConsoleCommandManager* GetCommandManager();

	ConsoleVariableManager* GetVariableManager();

	BindingManager* GetBindingManager();

	inline Context* GetFallbackContext()
	{
		return m_fallbackContext;
	}

private:
	Context* m_fallbackContext;

	int m_variableModifiedFlags;

	std::unique_ptr<ConsoleManagersBase> m_managers;

	std::string m_commandBuffer;

	std::mutex m_commandBufferMutex;
};

Context* GetDefaultContext();

void ExecuteSingleCommand(const std::string& command);

void ExecuteSingleCommand(const ProgramArguments& arguments);

ProgramArguments Tokenize(const std::string& line);

void AddToBuffer(const std::string& text);

void ExecuteBuffer();

void SaveConfigurationIfNeeded(const std::string& path);

void SetVariableModifiedFlags(int flags);
}
}