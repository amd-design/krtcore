#pragma once

#include <Console.Commands.h>
#include <ProgramArguments.h>

namespace krt
{
class ConsoleVariableManagerProxy
{
public:
	virtual ~ConsoleVariableManagerProxy() = default;
};

class ConsoleVariableManager;

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

	inline ConsoleCommandManager* GetCommandManager()
	{
		return m_commandManager.get();
	}

	inline ConsoleVariableManager* GetVariableManager()
	{
		return (ConsoleVariableManager*)m_variableManager.get();
	}

	inline Context* GetFallbackContext()
	{
		return m_fallbackContext;
	}

private:
	Context* m_fallbackContext;

	int m_variableModifiedFlags;

	std::unique_ptr<ConsoleCommandManager> m_commandManager;

	std::unique_ptr<ConsoleVariableManagerProxy> m_variableManager;

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