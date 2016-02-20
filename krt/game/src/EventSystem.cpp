#include <StdInc.h>
#include <EventSystem.h>

#include <sys/Timer.h>

#include <Console.h>

namespace krt
{
class NullEvent : public Event
{
  public:
	void Handle() override
	{
		// do nothing, this event isn't meant to be handled
	}
};

class ConsoleInputEvent : public Event
{
private:
	std::string m_text;

public:
	ConsoleInputEvent(const std::string& input)
		: m_text(input)
	{
		SetTime(sys::Milliseconds());
	}

	void Handle() override
	{
		console::AddToBuffer(m_text);
	}
};

static void ProcessConsoleInput(EventSystem* eventSystem)
{
	const char* consoleBuffer = sys::GetConsoleInput();

	if (consoleBuffer)
	{
		eventSystem->QueueEvent(std::make_unique<ConsoleInputEvent>(consoleBuffer));
	}
}

uint64_t EventSystem::HandleEvents()
{
	while (true)
	{
		ProcessConsoleInput(this);

		std::unique_ptr<Event> event = GetEvent();

		if (dynamic_cast<NullEvent*>(event.get()) != nullptr)
		{
			return event->GetTime();
		}

		event->Handle();
	}
}

std::unique_ptr<Event> EventSystem::GetEvent()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (!m_events.empty())
	{
		auto event = std::move(m_events.front());
		m_events.pop();

		return std::move(event);
	}

	// return the current time if we have no events anymore
	{
		auto event = std::make_unique<NullEvent>();
		event->SetTime(sys::Milliseconds());

		return std::move(event);
	}
}

void EventSystem::QueueEvent(std::unique_ptr<Event>&& ev)
{
	m_events.push(std::move(ev));
}
}