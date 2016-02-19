#pragma once

namespace krt
{
namespace sys
{
// gets the current time in milliseconds
uint64_t Milliseconds();

// a timer precision context (say, timeBeginPeriod on Windows)
class TimerContext
{
public:
	TimerContext();

	~TimerContext();
};
}
}