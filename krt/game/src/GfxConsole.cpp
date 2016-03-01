#include "StdInc.h"
#include <fonts/FontRenderer.h>

#include <EventSystem.h>

#include <Console.Commands.h>
#include <Console.Variables.h>
#include <Console.h>

#include <sstream>

namespace krt
{
class GfxConsole
{
  public:
	GfxConsole();

	void Render();

	void Print(const std::string& string);

  private:
	void HandleKey(const KeyEvent* ev);

	void HandleChar(const CharEvent* ev);

	void UpdateSuggestions();

	void SendCommand();

	void ResetTop();

  private:
	std::vector<std::string> m_screenBuffer;
	std::vector<std::string> m_commandHistory;

	std::mutex m_screenMutex;

	std::string m_inputBuffer;
	std::string m_typedInputBuffer;

	int m_screenWidth;
	int m_screenHeight;

	bool m_active;

	EventListener<KeyEvent> m_keyListener;
	EventListener<CharEvent> m_charListener;

	int m_screenTop;
	int m_screenSize;

	std::string m_suggestionSource;
	std::vector<std::string> m_suggestions;
	int m_currentSuggestion;
};

GfxConsole::GfxConsole()
    : m_screenWidth(1920), m_screenHeight(1080), m_active(false), m_keyListener(std::bind(&GfxConsole::HandleKey, this, std::placeholders::_1)), m_charListener(std::bind(&GfxConsole::HandleChar, this, std::placeholders::_1)), m_screenTop(0)
{
	// update width/height from console variables
	ConsoleVariableManager* varMan = console::GetDefaultContext()->GetVariableManager();
	m_screenWidth                  = atoi(varMan->FindEntryRaw("r_width")->GetValue().c_str());
	m_screenHeight                 = atoi(varMan->FindEntryRaw("r_height")->GetValue().c_str());

	// set size
	m_screenBuffer.resize(1);
	m_screenSize = 1;

	m_currentSuggestion = 0;
}

void GfxConsole::HandleKey(const KeyEvent* ev)
{
	if (!ev->IsDownEvent())
	{
		return;
	}

	KeyCode key = ev->GetKeyCode();

	// is this the console key?
	if (key == KeyCode::F11 || key == KeyCode::Oemtilde)
	{
		m_active = !m_active;

		if (m_active)
		{
			m_keyListener.Catch();
		}
		else
		{
			m_keyListener.Release();
		}
	}

	// return if not active
	if (!m_active)
	{
		return;
	}

	// backspace?
	if (key == KeyCode::Back)
	{
		if (m_inputBuffer.length() > 0)
		{
			m_inputBuffer      = m_inputBuffer.substr(0, m_inputBuffer.length() - 1);
			m_typedInputBuffer = m_inputBuffer;

			UpdateSuggestions();
		}
	}

	// submission key?
	if (key == KeyCode::Enter)
	{
		SendCommand();

		UpdateSuggestions();

		return;
	}

	// up/down for suggestions
	if (key == KeyCode::Up)
	{
		if (m_suggestions.size() > 0)
		{
			m_currentSuggestion--;

			if (m_currentSuggestion < 0)
			{
				m_currentSuggestion = m_suggestions.size() - 1;
			}

			m_inputBuffer = m_suggestions[m_currentSuggestion] + " ";
		}
	}

	if (key == KeyCode::Down)
	{
		if (m_suggestions.size() > 0)
		{
			m_currentSuggestion++;

			if (m_currentSuggestion >= m_suggestions.size())
			{
				m_currentSuggestion = 0;
			}

			m_inputBuffer = m_suggestions[m_currentSuggestion] + " ";
		}
	}

	// page up/page down
	if (key == KeyCode::PageUp)
	{
		m_screenTop -= 1;

		if (m_screenTop < 0)
		{
			m_screenTop = 0;
		}
	}

	if (key == KeyCode::PageDown)
	{
		std::lock_guard<std::mutex> lock(m_screenMutex);

		m_screenTop += 1;

		if (m_screenTop > m_screenBuffer.size())
		{
			m_screenTop = m_screenBuffer.size() - 1;
		}
	}
}

void GfxConsole::HandleChar(const CharEvent* ev)
{
	// ignore if not active
	if (!m_active)
	{
		return;
	}

	// ignore control characters
	if (ev->GetCharacter()[0] < 32)
	{
		return;
	}
	else if (ev->GetCharacter() == "`")
	{
		return;
	}

	m_inputBuffer += ev->GetCharacter();
	m_typedInputBuffer = m_inputBuffer;

	UpdateSuggestions();
}

void GfxConsole::UpdateSuggestions()
{
	if (m_typedInputBuffer.find(' ') != std::string::npos || m_typedInputBuffer.empty())
	{
		m_suggestions.clear();
		m_currentSuggestion = 0;

		m_suggestionSource = m_typedInputBuffer;

		return;
	}

	if (m_typedInputBuffer != m_suggestionSource)
	{
		std::set<std::string, IgnoreCaseLess> suggestions;

		console::GetDefaultContext()->GetCommandManager()->ForAllCommands([&](const std::string& name) {
			if (strnicmp(name.c_str(), m_typedInputBuffer.c_str(), m_typedInputBuffer.length()) != 0)
			{
				return;
			}

			if (stricmp(name.c_str(), m_typedInputBuffer.c_str()) == 0)
			{
				return;
			}

			suggestions.insert(name);
		});

		m_suggestions.resize(suggestions.size());
		std::move(suggestions.begin(), suggestions.end(), m_suggestions.begin());

		m_currentSuggestion = -1;

		m_suggestionSource = m_typedInputBuffer;
	}
}

void GfxConsole::SendCommand()
{
	Print("^7]" + m_inputBuffer + "\n");

	console::AddToBuffer(m_inputBuffer);
	console::AddToBuffer("\n");

	m_inputBuffer      = "";
	m_typedInputBuffer = "";

	UpdateSuggestions();
}

void GfxConsole::Print(const std::string& string)
{
	std::lock_guard<std::mutex> lock(m_screenMutex);

	std::stringstream stream;
	std::string* stringReference = &m_screenBuffer.back();

	for (int i = 0; i < string.length(); i++)
	{
		if (string[i] == '\n')
		{
			// append to the buffer and clear the stream
			*stringReference += stream.str();
			stream.str("");

			// add a new empty line
			m_screenBuffer.push_back("");
			stringReference = &m_screenBuffer.back();

			ResetTop();

			continue;
		}

		stream << string[i];
	}

	// append the remains
	*stringReference += stream.str();
}

void GfxConsole::ResetTop()
{
	m_screenTop = (m_screenBuffer.size() - m_screenSize);

	if (m_screenTop < 0)
	{
		m_screenTop = 0;
	}
}

void GfxConsole::Render()
{
	if (!m_active)
	{
		return;
	}

	int inputBoxRight;
	int inputBoxBottom;

	// input box
	{
		Rect mRect;

		std::string text = "KRT>^7 " + m_inputBuffer;
		TheFonts->GetStringMetrics(text, 14.0f, 1.0f, "Consolas", &mRect);

		int x = 15;
		int y = 15;

		int w = m_screenWidth - 30;
		int h = 16 + 3 + 3;

		Rect rect;
		rect.SetRect(x, y, x + w, y + h);

		TheFonts->DrawRectangle(rect, RGBA(0x22, 0x22, 0x22, 180));

		rect.SetRect(x + 1, y + 1, x + w - 1, y + h - 1);

		TheFonts->DrawRectangle(rect, RGBA(0x44, 0x44, 0x44, 180));

		rect.SetRect(x + 7, y + 3, 9999.0f, 9999.0f);

		TheFonts->DrawText(text, rect, RGBA(255, 255, 0), 14.0f, 1.0f, "Consolas");

		inputBoxRight  = x + 7 + mRect.Right();
		inputBoxBottom = y + h + 2;
	}

	// output box
	{
		std::lock_guard<std::mutex> lock(m_screenMutex);

		int x = 15;
		int y = inputBoxBottom + 5;

		int w = m_screenWidth - 30;
		int h = m_screenHeight - y - 15 - 2;

		Rect rect;
		rect.SetRect(x, y, x + w, y + h);

		TheFonts->DrawRectangle(rect, RGBA(0x22, 0x22, 0x22, 180));

		rect.SetRect(x + 1, y + 1, x + w - 1, y + h - 1);

		TheFonts->DrawRectangle(rect, RGBA(0x44, 0x44, 0x44, 180));

		int size = 16;

		// TODO: reset flag?

		m_screenSize = (h - 4) / size;
		int i        = 0;

		y += 5;

		for (auto it = m_screenBuffer.begin() + m_screenTop; it < m_screenBuffer.end(); it++)
		{
			rect.SetRect(x + 7, y, 9999.0, 9999.0);

			TheFonts->DrawText(*it, rect, RGBA(255, 255, 255), 14.0f, 1.0f, "Consolas");

			y += size;
			i++;

			if (i >= m_screenSize)
			{
				break;
			}
		}
	}

	// auto-completion
	{
		if (m_suggestions.size() > 0)
		{
			// append variable values and color strings
			std::vector<std::string> modifiedSuggestions;

			for (const std::string& line : m_suggestions)
			{
				std::string modifiedLine = line;

				// add convar value, if any
				auto variableEntry = console::GetDefaultContext()->GetVariableManager()->FindEntryRaw(modifiedLine);

				if (variableEntry.get())
				{
					modifiedLine += " " + variableEntry->GetValue();
				}

				// color the currently-written bit
				modifiedLine = "^3" + modifiedLine.substr(0, m_typedInputBuffer.length()) + "^7" + modifiedLine.substr(m_typedInputBuffer.length());

				modifiedSuggestions.push_back(modifiedLine);
			}

			// get the longest suggestion string
			auto longestSuggestion = std::max_element(modifiedSuggestions.begin(), modifiedSuggestions.end(), [](const std::string& left, const std::string& right) {
				return left.length() < right.length();
			});

			Rect mRect;
			TheFonts->GetStringMetrics(*longestSuggestion, 14.0f, 1.0f, "Consolas", &mRect);

			// set up coordinates
			int x = inputBoxRight;
			int y = inputBoxBottom - 2;

			int lines = std::min(m_suggestions.size(), (size_t)20);
			int h     = (lines * 16) + 4 + 6;
			int w     = mRect.Width() + 4 + 6;

			// draw background
			Rect rect;
			rect.SetRect(x, y, x + w, y + h);

			TheFonts->DrawRectangle(rect, RGBA(0x22, 0x22, 0x22, 180));

			rect.SetRect(x + 1, y + 1, x + w - 1, y + h - 1);

			TheFonts->DrawRectangle(rect, RGBA(0x44, 0x44, 0x44, 180));

			// loop the loop
			int i = 0;

			for (const std::string& line : modifiedSuggestions)
			{
				rect.SetRect(x + 5, y + 5, 9999.0, 9999.0);

				TheFonts->DrawText(line, rect, RGBA(255, 255, 255), 14.0f, 1.0f, "Consolas");

				i++;
				y += 16;

				if (i >= lines)
				{
					break;
				}
			}
		}
	}
}

GfxConsole* GetGfxConsole()
{
	static GfxConsole gfxConsole;
	static std::once_flag printListenerFlag;

	std::call_once(printListenerFlag, []() {
		console::AddPrintListener([](const char* string) {
			gfxConsole.Print(string);
		});
	});

	return &gfxConsole;
}

void RenderGfxConsole()
{
	GetGfxConsole()->Render();
}
}