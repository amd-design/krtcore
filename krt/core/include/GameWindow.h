#pragma once

namespace krt
{
class EventSystem;

class GameWindow abstract
{
public:
	virtual ~GameWindow() = default;

	virtual void* CreateGraphicsContext() = 0;

	virtual void Close() = 0;

	virtual void ProcessEvents() = 0;

	virtual void ProcessEventsOnce() = 0;

	virtual void GetMetrics(int& width, int& height) = 0;

public:
	static std::unique_ptr<GameWindow> Create(const std::string& title, int defaultWidth, int defaultHeight, EventSystem* eventSystem);
};
}