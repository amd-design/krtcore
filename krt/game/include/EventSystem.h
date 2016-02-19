#pragma once

#include <queue>

namespace krt
{
class Event abstract
{
  public:
	inline uint64_t GetTime() { return m_time; }

	inline void SetTime(uint64_t time) { m_time = time; }

  public:
	virtual void Handle() = 0;

  private:
	uint64_t m_time;
};

class EventSystem
{
  public:
	uint64_t HandleEvents();

	void QueueEvent(std::unique_ptr<Event>&& ev);

  private:
	std::unique_ptr<Event> GetEvent();

  private:
	std::queue<std::unique_ptr<Event>> m_events;

	std::mutex m_mutex;
};
}