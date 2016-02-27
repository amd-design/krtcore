#include <StdInc.h>
#include <GameWindow.h>

#include <Console.VariableHelpers.h>

#include <EventSystem.h>

#include <Game.h>

#include <windows.h>

namespace krt
{
const static KeyCode g_scancodeMapping[] = {
    KeyCode::None, KeyCode::Escape, KeyCode::D1, KeyCode::D2, KeyCode::D3, KeyCode::D4, KeyCode::D5, KeyCode::D6,
    KeyCode::D7, KeyCode::D8, KeyCode::D9, KeyCode::D0, KeyCode::OemMinus, KeyCode::Oemplus, KeyCode::Back, KeyCode::Tab,
    KeyCode::Q, KeyCode::W, KeyCode::E, KeyCode::R, KeyCode::T, KeyCode::Y, KeyCode::U, KeyCode::I,
    KeyCode::O, KeyCode::P, KeyCode::OemOpenBrackets, KeyCode::OemCloseBrackets, KeyCode::Return, KeyCode::ControlKey, KeyCode::A, KeyCode::S,
    KeyCode::D, KeyCode::F, KeyCode::G, KeyCode::H, KeyCode::J, KeyCode::K, KeyCode::L, KeyCode::OemSemicolon,
    KeyCode::OemQuotes, KeyCode::Oemtilde, KeyCode::LShiftKey, KeyCode::OemBackslash, KeyCode::Z, KeyCode::X, KeyCode::C, KeyCode::V,
    KeyCode::B, KeyCode::N, KeyCode::M, KeyCode::Oemcomma, KeyCode::OemPeriod, KeyCode::OemQuestion, KeyCode::RShiftKey, KeyCode::Multiply,
    KeyCode::Menu, KeyCode::Space, KeyCode::CapsLock, KeyCode::F1, KeyCode::F2, KeyCode::F3, KeyCode::F4, KeyCode::F5,
    KeyCode::F6, KeyCode::F7, KeyCode::F8, KeyCode::F9, KeyCode::F10, KeyCode::Pause, KeyCode::None, KeyCode::Home,
    KeyCode::Up, KeyCode::PageUp, KeyCode::Subtract, KeyCode::Left, KeyCode::NumPad5, KeyCode::Right, KeyCode::Add, KeyCode::End,
    KeyCode::Down, KeyCode::PageDown, KeyCode::Insert, KeyCode::Delete, KeyCode::None, KeyCode::None, KeyCode::None, KeyCode::F11,
    KeyCode::F12, KeyCode::None, KeyCode::None, KeyCode::None, KeyCode::None, KeyCode::None, KeyCode::None, KeyCode::None,
    KeyCode::None, KeyCode::None, KeyCode::None, KeyCode::None, KeyCode::None, KeyCode::None, KeyCode::None, KeyCode::None,
    KeyCode::None, KeyCode::None, KeyCode::None, KeyCode::None, KeyCode::None, KeyCode::None, KeyCode::None, KeyCode::None,
    KeyCode::None, KeyCode::None, KeyCode::None, KeyCode::None, KeyCode::None, KeyCode::None, KeyCode::None, KeyCode::None,
    KeyCode::None, KeyCode::None, KeyCode::None, KeyCode::None, KeyCode::None, KeyCode::None, KeyCode::None, KeyCode::None};

inline static KeyCode MapKeyCode(LPARAM lParam)
{
	// 16..23: scan code
	int scanCode = (lParam >> 16) & 0xFF;

	if (scanCode > 127)
	{
		return KeyCode::None;
	}

	// 24: is extended scan code?
	bool isExtendedKey = (lParam >> 24) & 1;

	KeyCode keyCode = g_scancodeMapping[scanCode];

	// extended mappings
	if (isExtendedKey)
	{
		switch (keyCode)
		{
		case KeyCode::OemQuestion:
			keyCode = KeyCode::Divide;
			break;
		case KeyCode::Pause:
			keyCode = KeyCode::NumLock;
			break;
		}
	}

	return keyCode;
}

void* InitializeD3D(HWND hWnd, int width, int height);

class Win32GameWindow : public GameWindow
{
  public:
	Win32GameWindow(const std::string& title, int defaultWidth, int defaultHeight, EventSystem* eventSystem)
	    : m_widthVar("r_width", ConVar_Archive, defaultWidth, &m_width),
	      m_heightVar("r_height", ConVar_Archive, defaultHeight, &m_height),
	      m_fullscreenVar("r_fullscreen", ConVar_Archive, false, &m_fullscreen),
	      m_eventSystem(eventSystem)
	{
		// prepare resolution
		HMONITOR hMonitor       = MonitorFromPoint(POINT{0, 0}, 0);
		MONITORINFO monitorInfo = {sizeof(MONITORINFO)};

		GetMonitorInfo(hMonitor, &monitorInfo);

		int x = 0;
		int y = 0;

		if (!m_fullscreen)
		{
			console::Printf("Creating %dx%d game window...\n", m_width, m_height);

			x = ((monitorInfo.rcWork.right - m_width) / 2);
			y = ((monitorInfo.rcWork.bottom - m_height) / 2);
		}
		else
		{
			m_widthVar.GetHelper()->SetRawValue(monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left);
			m_heightVar.GetHelper()->SetRawValue(monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top);

			console::Printf("Creating fullscreen (%dx%d) game window...\n", m_width, m_height);
		}

		// create the actual game window - well, the class
		{
			// TODO: icon resource
			WNDCLASS windowClass      = {0};
			windowClass.lpfnWndProc   = WindowProcedureWrapper;
			windowClass.hInstance     = GetModuleHandle(nullptr);
			windowClass.hCursor       = LoadCursor(0, IDC_ARROW);
			windowClass.lpszClassName = L"Win32GameWindow";

			check(RegisterClass(&windowClass));
		}

		DWORD windowStyle = WS_VISIBLE;

		if (m_fullscreen)
		{
			windowStyle |= WS_POPUP;
		}
		else
		{
			windowStyle |= WS_SYSMENU | WS_CAPTION;
		}

		// adjust the rectangle to fit a possible client area
		RECT windowRect;
		windowRect.left   = x;
		windowRect.right  = m_width + x;
		windowRect.top    = y;
		windowRect.bottom = m_height + y;

		AdjustWindowRect(&windowRect, windowStyle, FALSE);

		// create the window
		m_windowHandle = CreateWindowEx(0, L"Win32GameWindow", ToWide(title).c_str(),
		                                windowStyle, windowRect.left, windowRect.top, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
		                                nullptr, nullptr, GetModuleHandle(nullptr), nullptr);

		check(m_windowHandle != nullptr);

		ms_windowMapping.insert({m_windowHandle, this});
	}

	virtual ~Win32GameWindow() override
	{
		Close();
	}

	virtual void* CreateGraphicsContext() override
	{
		return InitializeD3D(m_windowHandle, m_width, m_height);
	}

	virtual void Close() override
	{
		DestroyWindow(m_windowHandle);
		UnregisterClass(L"Win32GameWindow", GetModuleHandle(nullptr));
	}

	virtual void ProcessEvents() override
	{
		MSG msg;

		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) != 0)
		{
			m_msgTime = msg.time;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

  private:
	LRESULT WindowProcedure(UINT msg, WPARAM wParam, LPARAM lParam)
	{
		int msgTimeDiff = (GetTickCount() - m_msgTime);
		uint64_t msgTime = theGame->GetGameTime() - msgTimeDiff;

		switch (msg)
		{
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		{
			auto event = std::make_unique<KeyEvent>(true, MapKeyCode(lParam));
			event->SetTime(msgTime);

			m_eventSystem->QueueEvent(std::move(event));
			break;
		}

		case WM_KEYUP:
		case WM_SYSKEYUP:
		{
			auto event = std::make_unique<KeyEvent>(false, MapKeyCode(lParam));
			event->SetTime(msgTime);

			m_eventSystem->QueueEvent(std::move(event));
			break;
		}

		case WM_CHAR:
		{
			wchar_t charParam = static_cast<wchar_t>(wParam);

			// WM_CHAR events are expected to be wchar_t, at least...
			m_eventSystem->QueueEvent(std::make_unique<CharEvent>(ToNarrow(std::wstring(&charParam, 1))));
			break;
		}

		case WM_CLOSE:
			PostQuitMessage(0);
			break;
		}

		return DefWindowProc(m_windowHandle, msg, wParam, lParam);
	}

  private:
	static std::map<HWND, Win32GameWindow*> ms_windowMapping;

	static LRESULT CALLBACK WindowProcedureWrapper(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		auto it = ms_windowMapping.find(window);

		if (it != ms_windowMapping.end())
		{
			return it->second->WindowProcedure(msg, wParam, lParam);
		}

		return DefWindowProc(window, msg, wParam, lParam);
	}

  private:
	ConVar<int> m_widthVar;
	ConVar<int> m_heightVar;

	ConVar<bool> m_fullscreenVar;

  private:
	EventSystem* m_eventSystem;

  private:
	HWND m_windowHandle;

	int m_width;
	int m_height;

	bool m_fullscreen;

	uint32_t m_msgTime;
};

std::map<HWND, Win32GameWindow*> Win32GameWindow::ms_windowMapping;

std::unique_ptr<GameWindow> GameWindow::Create(const std::string& title, int defaultWidth, int defaultHeight, EventSystem* eventSystem)
{
	return std::make_unique<Win32GameWindow>(title, defaultWidth, defaultHeight, eventSystem);
}
}