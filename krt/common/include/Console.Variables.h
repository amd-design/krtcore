#pragma once

#include <Console.CommandHelpers.h>
#include <Console.Commands.h>

namespace krt
{
namespace internal
{
class ConsoleVariableEntryBase
{
public:
	virtual std::string GetValue() = 0;

	virtual bool SetValue(const std::string& value) = 0;
};

template <typename T, typename TConstraint = void>
struct Constraints
{
	inline static bool Compare(const T& value, const T& minValue, const T& maxValue)
	{
		return true;
	}
};

template <typename T>
struct Constraints<T, std::enable_if_t<std::is_arithmetic<T>::value>>
{
	inline static bool Compare(const T& value, const T& minValue, const T& maxValue)
	{
		if (ConsoleArgumentTraits<T>::Greater()(value, maxValue))
		{
			console::Printf("Value out of range (%s) - should be at most %s\n", UnparseArgument(value).c_str(), UnparseArgument(maxValue).c_str());
			return false;
		}

		if (ConsoleArgumentTraits<T>::Less()(value, minValue))
		{
			console::Printf("Value out of range (%s) - should be at least %s\n", UnparseArgument(value).c_str(), UnparseArgument(minValue).c_str());
			return false;
		}

		return true;
	}
};

void MarkConsoleVarModified(ConsoleVariableManager* manager, const std::string& name);

template <typename T>
class ConsoleVariableEntry : public ConsoleVariableEntryBase
{
public:
	ConsoleVariableEntry(ConsoleVariableManager* manager, const std::string& name, const T& defaultValue)
	    : m_manager(manager), m_name(name), m_trackingVar(nullptr), m_defaultValue(defaultValue), m_curValue(defaultValue), m_hasConstraints(false)
	{
		m_getCommand = std::make_unique<ConsoleCommand>(manager->GetParentContext(), name, [=]() {
			console::Printf(" \"%s\" is \"%s\"\n default: \"%s\"\n type: %s\n", name.c_str(), GetValue().c_str(), UnparseArgument(m_defaultValue).c_str(), ConsoleArgumentName<T>::Get());
		});

		m_setCommand = std::make_unique<ConsoleCommand>(manager->GetParentContext(), name, [=](const T& newValue) {
			SetRawValue(newValue);
		});
	}

	inline void SetConstraints(const T& minValue, const T& maxValue)
	{
		m_minValue = minValue;
		m_maxValue = maxValue;

		m_hasConstraints = true;
	}

	inline void SetTrackingVar(T* variable)
	{
		m_trackingVar = variable;

		if (variable)
		{
			*variable = m_curValue;
		}
	}

	virtual std::string GetValue() override
	{
		// update from a tracking variable
		if (m_trackingVar)
		{
			if (!ConsoleArgumentTraits<T>::Equal()(*m_trackingVar, m_curValue))
			{
				m_curValue = *m_trackingVar;
			}
		}

		return UnparseArgument(m_curValue);
	}

	virtual bool SetValue(const std::string& value) override
	{
		T newValue;

		if (ParseArgument(value, &newValue))
		{
			return SetRawValue(newValue);
		}

		return false;
	}

	inline const T& GetRawValue()
	{
		return m_curValue;
	}

	inline bool SetRawValue(const T& newValue)
	{
		if (m_hasConstraints && !Constraints<T>::Compare(newValue, m_minValue, m_maxValue))
		{
			return false;
		}

		// update modified flags if changed
		if (!ConsoleArgumentTraits<T>::Equal()(m_curValue, newValue))
		{
			// indirection as manager isn't declared by now
			MarkConsoleVarModified(m_manager, m_name);
		}

		m_curValue = newValue;

		if (m_trackingVar)
		{
			*m_trackingVar = m_curValue;
		}

		return true;
	}

private:
	std::string m_name;

	T m_curValue;

	T m_minValue;
	T m_maxValue;

	T m_defaultValue;

	T* m_trackingVar;

	bool m_hasConstraints;

	std::unique_ptr<ConsoleCommand> m_getCommand;
	std::unique_ptr<ConsoleCommand> m_setCommand;

	ConsoleVariableManager* m_manager;
};
}

enum ConsoleVariableFlags
{
	ConVar_None     = 0,
	ConVar_Archive  = 0x1,
	ConVar_Modified = 2
};

class ConsoleVariableManager
{
public:
	using THandlerPtr = std::shared_ptr<internal::ConsoleVariableEntryBase>;

	using TVariableCB = std::function<void(const std::string& name, int flags, const THandlerPtr& variable)>;

	using TWriteLineCB = std::function<void(const std::string& line)>;

public:
	ConsoleVariableManager(console::Context* context);

	~ConsoleVariableManager();

	int Register(const std::string& name, int flags, const THandlerPtr& variable);

	void Unregister(int token);

	bool Process(const std::string& commandName, const ProgramArguments& arguments);

	THandlerPtr FindEntryRaw(const std::string& name);

	void AddEntryFlags(const std::string& name, int flags);

	void RemoveEntryFlags(const std::string& name, int flags);

	void ForAllVariables(const TVariableCB& callback, int flagMask = 0xFFFFFFFF);

	void SaveConfiguration(const TWriteLineCB& writeLineFunction);

	inline console::Context* GetParentContext()
	{
		return m_parentContext;
	}

private:
	struct Entry
	{
		std::string name;

		int flags;

		THandlerPtr variable;

		int token;

		inline Entry(const std::string& name, int flags, const THandlerPtr& variable, int token)
		    : name(name), flags(flags), variable(variable), token(token)
		{
		}
	};

private:
	console::Context* m_parentContext;

	std::map<std::string, Entry, IgnoreCaseLess> m_entries;

	std::shared_timed_mutex m_mutex;

	std::atomic<int> m_curToken;

	std::unique_ptr<ConsoleCommand> m_setCommand;
	std::unique_ptr<ConsoleCommand> m_setaCommand;

	std::unique_ptr<ConsoleCommand> m_toggleCommand;
	std::unique_ptr<ConsoleCommand> m_vstrCommand;

public:
	static ConsoleVariableManager* GetDefaultInstance();
};

template <typename TValue>
static std::shared_ptr<internal::ConsoleVariableEntry<TValue>> CreateVariableEntry(ConsoleVariableManager* manager, const std::string& name, const TValue& defaultValue)
{
	ConsoleVariableManager::THandlerPtr oldEntry = manager->FindEntryRaw(name);

	// try to return/cast an old entry
	if (oldEntry)
	{
		// if this is already an entry of the right type, return said entry
		auto oldType = std::dynamic_pointer_cast<internal::ConsoleVariableEntry<TValue>>(oldEntry);

		if (oldType)
		{
			return oldType;
		}

		// not the same type, create a new entry we'll hopefully treat better
		std::string oldValue = oldEntry->GetValue();

		auto newEntry = std::make_shared<internal::ConsoleVariableEntry<TValue>>(manager, name, defaultValue);
		newEntry->SetValue(oldValue);

		return newEntry;
	}
	else
	{
		// no old entry exists, just create a default
		auto newEntry = std::make_shared<internal::ConsoleVariableEntry<TValue>>(manager, name, defaultValue);

		return newEntry;
	}
}
}