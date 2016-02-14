#include <StdInc.h>
#include <Console.Commands.h>

#include <utils/LockUtil.h>
#include <utils/IteratorView.h>

namespace krt
{
ConsoleCommandManager::ConsoleCommandManager()
{

}

ConsoleCommandManager::~ConsoleCommandManager()
{

}

void ConsoleCommandManager::Register(const std::string& name, const THandler& handler)
{
	auto lock = exclusive_lock_acquire<std::shared_timed_mutex>(m_mutex);

	m_entries.insert({ name, Entry{name, handler} });
}

void ConsoleCommandManager::Invoke(const std::string& commandString)
{
	assert(!"Not implemented (needs tokenizer)");
}

void ConsoleCommandManager::Invoke(const std::string& commandName, const ProgramArguments& arguments)
{
	// lock the mutex
	auto lock = shared_lock_acquire<std::shared_timed_mutex>(m_mutex);

	// acquire a list of command entries
	auto entryPair = m_entries.equal_range(commandName);

	if (entryPair.first == entryPair.second)
	{
		// TODO: replace with console stream output
		printf("No such command %s.\n", commandName.c_str());
		return;
	}

	// try executing overloads until finding one that accepts our arguments - if none is found, print the error buffer
	ConsoleExecutionContext context(std::move(arguments));
	bool result = false;

	for (std::pair<const std::string, Entry>& entry : GetIteratorView(entryPair))
	{
		result = entry.second.function(context);

		if (result)
		{
			break;
		}
	}

	if (!result)
	{
		printf("%s", context.errorBuffer.str().c_str());
	}
}

static ConsoleCommandManager g_manager;
ConsoleCommandManager* ConsoleCommandManager::ms_instance = &g_manager;
}