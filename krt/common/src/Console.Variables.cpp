#include <StdInc.h>
#include <Console.h>
#include <Console.Variables.h>

namespace krt
{
ConsoleVariableManager::ConsoleVariableManager(console::Context* parentContext)
	: m_parentContext(parentContext), m_curToken(0)
{
	auto setCommand = [=] (bool archive, const std::string& variable, const std::string& value)
	{
		// weird order is to prevent recursive locking
		{
			auto lock = shared_lock_acquire<std::shared_timed_mutex>(m_mutex);

			auto oldVariable = m_entries.find(variable);

			if (oldVariable != m_entries.end())
			{
				oldVariable->second.variable->SetValue(value);

				if (archive)
				{
					oldVariable->second.flags |= ConVar_Archive;
				}

				return;
			}
		}

		int flags = ConVar_None;

		if (archive)
		{
			flags |= ConVar_Archive;
		}

		auto entry = CreateVariableEntry<std::string>(this, variable, "");
		entry->SetValue(value);

		Register(variable, flags, entry);
	};

	m_setCommand = std::make_unique<ConsoleCommand>(m_parentContext, "set", [=] (const std::string& variable, const std::string& value)
	{
		setCommand(false, variable, value);
	});

	// set archived
	m_setaCommand = std::make_unique<ConsoleCommand>(m_parentContext, "seta", [=] (const std::string& variable, const std::string& value)
	{
		setCommand(true, variable, value);
	});
}

ConsoleVariableManager::~ConsoleVariableManager()
{
}

ConsoleVariableManager::THandlerPtr ConsoleVariableManager::FindEntryRaw(const std::string& name)
{
	auto variable = m_entries.find(name);

	if (variable != m_entries.end())
	{
		return variable->second.variable;
	}

	return nullptr;
}

int ConsoleVariableManager::Register(const std::string& name, int flags, const THandlerPtr& variable)
{
	auto lock = exclusive_lock_acquire<std::shared_timed_mutex>(m_mutex);

	int token = m_curToken.fetch_add(1);
	m_entries.erase(name); // remove any existing entry

	m_entries.insert({ name, Entry{name, flags, variable, token} });

	return token;
}

void ConsoleVariableManager::Unregister(int token)
{
	auto lock = exclusive_lock_acquire<std::shared_timed_mutex>(m_mutex);

	// look through the list for a matching token
	for (auto it = m_entries.begin(); it != m_entries.end(); it++)
	{
		if (it->second.token == token)
		{
			// erase and return immediately (so we won't increment a bad iterator)
			m_entries.erase(it);
			return;
		}
	}
}

bool ConsoleVariableManager::Process(const std::string& commandName, const ProgramArguments& arguments)
{
	// we currently don't process any variables specifically
	return false;
}

ConsoleVariableManager* ConsoleVariableManager::GetDefaultInstance()
{
	return console::GetDefaultContext()->GetVariableManager();
}
}