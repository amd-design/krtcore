#pragma once

#include "Console.CommandHelpers.h"

#include "EventSystem.h"

namespace krt
{
class Button
{
public:
	Button(const std::string& name);

	inline const std::string& GetName() const
	{
		return m_name;
	}

	inline bool IsDown() const
	{
		return m_isDown;
	}

	float GetPressedFraction() const;

private:
	std::string m_name;

	bool m_isDown;

	KeyCode m_keys[2];
	uint64_t m_pressTime;

	std::unique_ptr<ConsoleCommand> m_downCommand;
	std::unique_ptr<ConsoleCommand> m_downCommandWithTime;

	std::unique_ptr<ConsoleCommand> m_upCommand;
	std::unique_ptr<ConsoleCommand> m_upCommandWithKey;
};

class BindingManager
{
public:
	using TWriteLineCB = std::function<void(const std::string& line)>;

public:
	BindingManager(console::Context* context);

	void WriteBindings(const TWriteLineCB& writeLineFunction);

	void HandleKeyEvent(const KeyEvent* ev);

private:
	std::unique_ptr<ConsoleCommand> m_bindCommand;
	std::unique_ptr<ConsoleCommand> m_unbindCommand;

	std::unique_ptr<ConsoleCommand> m_unbindAllCommand;

	std::unique_ptr<EventListener<KeyEvent>> m_keyListener;

	std::map<KeyCode, std::string> m_bindings;
};

extern BindingManager* g_bindings;
}