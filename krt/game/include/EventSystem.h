#pragma once

#include <queue>

namespace krt
{
class Event abstract
{
public:
	inline uint64_t GetTime() const { return m_time; }

	inline void SetTime(uint64_t time) { m_time = time; }

public:
	virtual void Handle() = 0;

private:
	uint64_t m_time;
};

enum class KeyCode
{
	None               = 0x00000000,
	LButton            = 0x00000001,
	RButton            = 0x00000002,
	Cancel             = 0x00000003,
	MButton            = 0x00000004,
	XButton1           = 0x00000005,
	XButton2           = 0x00000006,
	Back               = 0x00000008,
	Tab                = 0x00000009,
	LineFeed           = 0x0000000A,
	Clear              = 0x0000000C,
	Return             = 0x0000000D,
	Enter              = 0x0000000D,
	ShiftKey           = 0x00000010,
	ControlKey         = 0x00000011,
	Menu               = 0x00000012,
	Pause              = 0x00000013,
	CapsLock           = 0x00000014,
	Capital            = 0x00000014,
	KanaMode           = 0x00000015,
	HanguelMode        = 0x00000015,
	HangulMode         = 0x00000015,
	JunjaMode          = 0x00000017,
	FinalMode          = 0x00000018,
	KanjiMode          = 0x00000019,
	HanjaMode          = 0x00000019,
	Escape             = 0x0000001B,
	IMEConvert         = 0x0000001C,
	IMENonconvert      = 0x0000001D,
	IMEAceept          = 0x0000001E,
	IMEModeChange      = 0x0000001F,
	Space              = 0x00000020,
	PageUp             = 0x00000021,
	Prior              = 0x00000021,
	PageDown           = 0x00000022,
	Next               = 0x00000022,
	End                = 0x00000023,
	Home               = 0x00000024,
	Left               = 0x00000025,
	Up                 = 0x00000026,
	Right              = 0x00000027,
	Down               = 0x00000028,
	Select             = 0x00000029,
	Print              = 0x0000002A,
	Execute            = 0x0000002B,
	PrintScreen        = 0x0000002C,
	Snapshot           = 0x0000002C,
	Insert             = 0x0000002D,
	Delete             = 0x0000002E,
	Help               = 0x0000002F,
	D0                 = 0x00000030,
	D1                 = 0x00000031,
	D2                 = 0x00000032,
	D3                 = 0x00000033,
	D4                 = 0x00000034,
	D5                 = 0x00000035,
	D6                 = 0x00000036,
	D7                 = 0x00000037,
	D8                 = 0x00000038,
	D9                 = 0x00000039,
	A                  = 0x00000041,
	B                  = 0x00000042,
	C                  = 0x00000043,
	D                  = 0x00000044,
	E                  = 0x00000045,
	F                  = 0x00000046,
	G                  = 0x00000047,
	H                  = 0x00000048,
	I                  = 0x00000049,
	J                  = 0x0000004A,
	K                  = 0x0000004B,
	L                  = 0x0000004C,
	M                  = 0x0000004D,
	N                  = 0x0000004E,
	O                  = 0x0000004F,
	P                  = 0x00000050,
	Q                  = 0x00000051,
	R                  = 0x00000052,
	S                  = 0x00000053,
	T                  = 0x00000054,
	U                  = 0x00000055,
	V                  = 0x00000056,
	W                  = 0x00000057,
	X                  = 0x00000058,
	Y                  = 0x00000059,
	Z                  = 0x0000005A,
	LWin               = 0x0000005B,
	RWin               = 0x0000005C,
	Apps               = 0x0000005D,
	NumPad0            = 0x00000060,
	NumPad1            = 0x00000061,
	NumPad2            = 0x00000062,
	NumPad3            = 0x00000063,
	NumPad4            = 0x00000064,
	NumPad5            = 0x00000065,
	NumPad6            = 0x00000066,
	NumPad7            = 0x00000067,
	NumPad8            = 0x00000068,
	NumPad9            = 0x00000069,
	Multiply           = 0x0000006A,
	Add                = 0x0000006B,
	Separator          = 0x0000006C,
	Subtract           = 0x0000006D,
	Decimal            = 0x0000006E,
	Divide             = 0x0000006F,
	F1                 = 0x00000070,
	F2                 = 0x00000071,
	F3                 = 0x00000072,
	F4                 = 0x00000073,
	F5                 = 0x00000074,
	F6                 = 0x00000075,
	F7                 = 0x00000076,
	F8                 = 0x00000077,
	F9                 = 0x00000078,
	F10                = 0x00000079,
	F11                = 0x0000007A,
	F12                = 0x0000007B,
	F13                = 0x0000007C,
	F14                = 0x0000007D,
	F15                = 0x0000007E,
	F16                = 0x0000007F,
	F17                = 0x00000080,
	F18                = 0x00000081,
	F19                = 0x00000082,
	F20                = 0x00000083,
	F21                = 0x00000084,
	F22                = 0x00000085,
	F23                = 0x00000086,
	F24                = 0x00000087,
	NumLock            = 0x00000090,
	Scroll             = 0x00000091,
	LShiftKey          = 0x000000A0,
	RShiftKey          = 0x000000A1,
	LControlKey        = 0x000000A2,
	RControlKey        = 0x000000A3,
	LMenu              = 0x000000A4,
	RMenu              = 0x000000A5,
	BrowserBack        = 0x000000A6,
	BrowserForward     = 0x000000A7,
	BrowserRefresh     = 0x000000A8,
	BrowserStop        = 0x000000A9,
	BrowserSearch      = 0x000000AA,
	BrowserFavorites   = 0x000000AB,
	BrowserHome        = 0x000000AC,
	VolumeMute         = 0x000000AD,
	VolumeDown         = 0x000000AE,
	VolumeUp           = 0x000000AF,
	MediaNextTrack     = 0x000000B0,
	MediaPreviousTrack = 0x000000B1,
	MediaStop          = 0x000000B2,
	MediaPlayPause     = 0x000000B3,
	LaunchMail         = 0x000000B4,
	SelectMedia        = 0x000000B5,
	LaunchApplication1 = 0x000000B6,
	LaunchApplication2 = 0x000000B7,
	OemSemicolon       = 0x000000BA,
	Oemplus            = 0x000000BB,
	Oemcomma           = 0x000000BC,
	OemMinus           = 0x000000BD,
	OemPeriod          = 0x000000BE,
	OemQuestion        = 0x000000BF,
	Oemtilde           = 0x000000C0,
	OemOpenBrackets    = 0x000000DB,
	OemPipe            = 0x000000DC,
	OemCloseBrackets   = 0x000000DD,
	OemQuotes          = 0x000000DE,
	Oem8               = 0x000000DF,
	OemBackslash       = 0x000000E2,
	ProcessKey         = 0x000000E5,
	Attn               = 0x000000F6,
	Crsel              = 0x000000F7,
	Exsel              = 0x000000F8,
	EraseEof           = 0x000000F9,
	Play               = 0x000000FA,
	Zoom               = 0x000000FB,
	NoName             = 0x000000FC,
	Pa1                = 0x000000FD,
	OemClear           = 0x000000FE,
	IMEAccept          = 0x0000001E,
	Oem1               = 0x000000BA,
	Oem102             = 0x000000E2,
	Oem2               = 0x000000BF,
	Oem3               = 0x000000C0,
	Oem4               = 0x000000DB,
	Oem5               = 0x000000DC,
	Oem6               = 0x000000DD,
	Oem7               = 0x000000DE,
	Packet             = 0x000000E7,
	Sleep              = 0x0000005F
};

class KeyEvent : public Event
{
public:
	inline KeyEvent(bool isDown, KeyCode keyCode)
	    : m_isDown(isDown), m_keyCode(keyCode)
	{
	}

	virtual void Handle() override;

	inline bool IsDownEvent() const
	{
		return m_isDown;
	}

	inline KeyCode GetKeyCode() const
	{
		return m_keyCode;
	}

private:
	bool m_isDown;

	KeyCode m_keyCode;
};

// actually [arbitrary UTF8 sequence] event, thanks input methods!
class CharEvent : public Event
{
public:
	inline CharEvent(const std::string& ch)
	    : m_character(ch)
	{
	}

	virtual void Handle() override;

	inline const std::string& GetCharacter() const { return m_character; }

private:
	std::string m_character;
};

class MouseEvent : public Event
{
public:
	inline MouseEvent(int dX, int dY)
	    : m_dX(dX), m_dY(dY)
	{
	}

	virtual void Handle() override;

	inline int GetDeltaX() const
	{
		return m_dX;
	}

	inline int GetDeltaY() const
	{
		return m_dY;
	}

private:
	int m_dX;
	int m_dY;
};

template <typename T>
struct EventListener
{
	struct State
	{
		std::set<EventListener*> listeners;
		EventListener* catcher;

		State()
		    : catcher(nullptr)
		{
		}

		~State()
		{
			for (auto& listener : listeners)
			{
				// so we won't try destructing
				listener->m_hasListener = false;
			}
		}
	};

	template <typename TCallback>
	EventListener(const TCallback& callback)
		: m_hasListener(true)
	{
		m_handler = callback;

		GetState()->listeners.insert(this);
	}

	~EventListener()
	{
		if (m_hasListener)
		{
			GetState()->listeners.erase(this);
		}
	}

	void Catch()
	{
		assert(GetState()->catcher == nullptr);

		GetState()->catcher = this;
	}

	void Release()
	{
		assert(GetState()->catcher == this);

		GetState()->catcher = nullptr;
	}

private:
	std::function<void(const T*)> m_handler;

	bool m_hasListener;

	// static members
public:
	static void Handle(const T* event)
	{
		for (auto& listener : GetState()->listeners)
		{
			// ignore if no catcher is set
			if (GetState()->catcher && GetState()->catcher != listener)
			{
				continue;
			}

			listener->m_handler(event);
		}
	}

private:
	// this is not a dynamic initializer due to initializer order
	static State* GetState()
	{
		static State state;

		return &state;
	}
};

#define DECLARE_EVENT_LISTENER(t)

class EventSystem
{
public:
	uint64_t HandleEvents();

	void QueueEvent(std::unique_ptr<Event>&& ev);

	void RegisterEventSourceFunction(const std::function<void()>& function);

private:
	std::unique_ptr<Event> GetEvent();

private:
	std::vector<std::function<void()>> m_eventSources;

	std::queue<std::unique_ptr<Event>> m_events;

	std::mutex m_mutex;
};
}