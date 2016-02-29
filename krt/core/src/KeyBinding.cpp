#include "StdInc.h"
#include "KeyBinding.h"

#include "Game.h"

namespace krt
{
Button::Button(const std::string& name)
	: m_name(name), m_isDown(false), m_pressTime(0)
{
	m_downCommand = std::make_unique<ConsoleCommand>("+" + name, [=] ()
	{
		m_pressTime = 0;
		m_isDown = true;
	});

	m_downCommandWithTime = std::make_unique<ConsoleCommand>("+" + name, [=] (int keyCode, uint64_t eventTime)
	{
		KeyCode enumCode = static_cast<KeyCode>(keyCode);

		if (m_keys[0] == KeyCode::None || m_keys[0] == enumCode)
		{
			m_pressTime = eventTime;
			m_keys[0] = enumCode;
		}
		else if (m_keys[1] == KeyCode::None || m_keys[1] == enumCode)
		{
			m_pressTime = eventTime;
			m_keys[1] = enumCode;
		}
		else
		{
			console::Printf("Three keys down for a button!\n");
			return;
		}

		m_isDown = true;
	});

	m_upCommand = std::make_unique<ConsoleCommand>("-" + name, [=] ()
	{
		m_keys[0] = KeyCode::None;
		m_keys[1] = KeyCode::None;

		m_isDown = false;
	});

	m_upCommandWithKey = std::make_unique<ConsoleCommand>("-" + name, [=] (int keyCode)
	{
		KeyCode enumCode = static_cast<KeyCode>(keyCode);

		if (m_keys[0] == enumCode)
		{
			m_keys[0] = m_keys[1];
			m_keys[1] = KeyCode::None;
		}
		else if (m_keys[1] == enumCode)
		{
			m_keys[1] = KeyCode::None;
		}

		if (m_keys[0] == KeyCode::None && m_keys[1] == KeyCode::None)
		{
			m_isDown = false;
		}
	});

	m_keys[0] = KeyCode::None;
	m_keys[1] = KeyCode::None;
}

float Button::GetPressedFraction() const
{
	if (!m_isDown)
	{
		return 0.0f;
	}

	float fraction = (theGame->GetGameTime() - m_pressTime) / static_cast<float>(theGame->GetLastFrameTime());

	if (fraction > 1.0f)
	{
		fraction = 1.0f;
	}
	else if (fraction < 0.0f)
	{
		fraction = 0.0f;
	}

	return fraction;
}

static std::map<std::string, KeyCode, IgnoreCaseLess> keyNameMap =
{
	{ "None", KeyCode::None },
	{ "LButton", KeyCode::LButton },
	{ "RButton", KeyCode::RButton },
	{ "Cancel", KeyCode::Cancel },
	{ "MButton", KeyCode::MButton },
	{ "XButton1", KeyCode::XButton1 },
	{ "XButton2", KeyCode::XButton2 },
	{ "Back", KeyCode::Back },
	{ "Tab", KeyCode::Tab },
	{ "LineFeed", KeyCode::LineFeed },
	{ "Clear", KeyCode::Clear },
	{ "Return", KeyCode::Return },
	{ "Enter", KeyCode::Enter },
	{ "ShiftKey", KeyCode::ShiftKey },
	{ "ControlKey", KeyCode::ControlKey },
	{ "Menu", KeyCode::Menu },
	{ "Pause", KeyCode::Pause },
	{ "CapsLock", KeyCode::CapsLock },
	{ "Capital", KeyCode::Capital },
	{ "KanaMode", KeyCode::KanaMode },
	{ "HanguelMode", KeyCode::HanguelMode },
	{ "HangulMode", KeyCode::HangulMode },
	{ "JunjaMode", KeyCode::JunjaMode },
	{ "FinalMode", KeyCode::FinalMode },
	{ "KanjiMode", KeyCode::KanjiMode },
	{ "HanjaMode", KeyCode::HanjaMode },
	{ "Escape", KeyCode::Escape },
	{ "IMEConvert", KeyCode::IMEConvert },
	{ "IMENonconvert", KeyCode::IMENonconvert },
	{ "IMEAceept", KeyCode::IMEAceept },
	{ "IMEModeChange", KeyCode::IMEModeChange },
	{ "Space", KeyCode::Space },
	{ "PageUp", KeyCode::PageUp },
	{ "Prior", KeyCode::Prior },
	{ "PageDown", KeyCode::PageDown },
	{ "Next", KeyCode::Next },
	{ "End", KeyCode::End },
	{ "Home", KeyCode::Home },
	{ "Left", KeyCode::Left },
	{ "Up", KeyCode::Up },
	{ "Right", KeyCode::Right },
	{ "Down", KeyCode::Down },
	{ "Select", KeyCode::Select },
	{ "Print", KeyCode::Print },
	{ "Execute", KeyCode::Execute },
	{ "PrintScreen", KeyCode::PrintScreen },
	{ "Snapshot", KeyCode::Snapshot },
	{ "Insert", KeyCode::Insert },
	{ "Delete", KeyCode::Delete },
	{ "Help", KeyCode::Help },
	{ "D0", KeyCode::D0 },
	{ "D1", KeyCode::D1 },
	{ "D2", KeyCode::D2 },
	{ "D3", KeyCode::D3 },
	{ "D4", KeyCode::D4 },
	{ "D5", KeyCode::D5 },
	{ "D6", KeyCode::D6 },
	{ "D7", KeyCode::D7 },
	{ "D8", KeyCode::D8 },
	{ "D9", KeyCode::D9 },
	{ "A", KeyCode::A },
	{ "B", KeyCode::B },
	{ "C", KeyCode::C },
	{ "D", KeyCode::D },
	{ "E", KeyCode::E },
	{ "F", KeyCode::F },
	{ "G", KeyCode::G },
	{ "H", KeyCode::H },
	{ "I", KeyCode::I },
	{ "J", KeyCode::J },
	{ "K", KeyCode::K },
	{ "L", KeyCode::L },
	{ "M", KeyCode::M },
	{ "N", KeyCode::N },
	{ "O", KeyCode::O },
	{ "P", KeyCode::P },
	{ "Q", KeyCode::Q },
	{ "R", KeyCode::R },
	{ "S", KeyCode::S },
	{ "T", KeyCode::T },
	{ "U", KeyCode::U },
	{ "V", KeyCode::V },
	{ "W", KeyCode::W },
	{ "X", KeyCode::X },
	{ "Y", KeyCode::Y },
	{ "Z", KeyCode::Z },
	{ "LWin", KeyCode::LWin },
	{ "RWin", KeyCode::RWin },
	{ "Apps", KeyCode::Apps },
	{ "NumPad0", KeyCode::NumPad0 },
	{ "NumPad1", KeyCode::NumPad1 },
	{ "NumPad2", KeyCode::NumPad2 },
	{ "NumPad3", KeyCode::NumPad3 },
	{ "NumPad4", KeyCode::NumPad4 },
	{ "NumPad5", KeyCode::NumPad5 },
	{ "NumPad6", KeyCode::NumPad6 },
	{ "NumPad7", KeyCode::NumPad7 },
	{ "NumPad8", KeyCode::NumPad8 },
	{ "NumPad9", KeyCode::NumPad9 },
	{ "Multiply", KeyCode::Multiply },
	{ "Add", KeyCode::Add },
	{ "Separator", KeyCode::Separator },
	{ "Subtract", KeyCode::Subtract },
	{ "Decimal", KeyCode::Decimal },
	{ "Divide", KeyCode::Divide },
	{ "F1", KeyCode::F1 },
	{ "F2", KeyCode::F2 },
	{ "F3", KeyCode::F3 },
	{ "F4", KeyCode::F4 },
	{ "F5", KeyCode::F5 },
	{ "F6", KeyCode::F6 },
	{ "F7", KeyCode::F7 },
	{ "F8", KeyCode::F8 },
	{ "F9", KeyCode::F9 },
	{ "F10", KeyCode::F10 },
	{ "F11", KeyCode::F11 },
	{ "F12", KeyCode::F12 },
	{ "F13", KeyCode::F13 },
	{ "F14", KeyCode::F14 },
	{ "F15", KeyCode::F15 },
	{ "F16", KeyCode::F16 },
	{ "F17", KeyCode::F17 },
	{ "F18", KeyCode::F18 },
	{ "F19", KeyCode::F19 },
	{ "F20", KeyCode::F20 },
	{ "F21", KeyCode::F21 },
	{ "F22", KeyCode::F22 },
	{ "F23", KeyCode::F23 },
	{ "F24", KeyCode::F24 },
	{ "NumLock", KeyCode::NumLock },
	{ "Scroll", KeyCode::Scroll },
	{ "LShiftKey", KeyCode::LShiftKey },
	{ "RShiftKey", KeyCode::RShiftKey },
	{ "LControlKey", KeyCode::LControlKey },
	{ "RControlKey", KeyCode::RControlKey },
	{ "LMenu", KeyCode::LMenu },
	{ "RMenu", KeyCode::RMenu },
	{ "BrowserBack", KeyCode::BrowserBack },
	{ "BrowserForward", KeyCode::BrowserForward },
	{ "BrowserRefresh", KeyCode::BrowserRefresh },
	{ "BrowserStop", KeyCode::BrowserStop },
	{ "BrowserSearch", KeyCode::BrowserSearch },
	{ "BrowserFavorites", KeyCode::BrowserFavorites },
	{ "BrowserHome", KeyCode::BrowserHome },
	{ "VolumeMute", KeyCode::VolumeMute },
	{ "VolumeDown", KeyCode::VolumeDown },
	{ "VolumeUp", KeyCode::VolumeUp },
	{ "MediaNextTrack", KeyCode::MediaNextTrack },
	{ "MediaPreviousTrack", KeyCode::MediaPreviousTrack },
	{ "MediaStop", KeyCode::MediaStop },
	{ "MediaPlayPause", KeyCode::MediaPlayPause },
	{ "LaunchMail", KeyCode::LaunchMail },
	{ "SelectMedia", KeyCode::SelectMedia },
	{ "LaunchApplication1", KeyCode::LaunchApplication1 },
	{ "LaunchApplication2", KeyCode::LaunchApplication2 },
	{ "OemSemicolon", KeyCode::OemSemicolon },
	{ "Oemplus", KeyCode::Oemplus },
	{ "Oemcomma", KeyCode::Oemcomma },
	{ "OemMinus", KeyCode::OemMinus },
	{ "OemPeriod", KeyCode::OemPeriod },
	{ "OemQuestion", KeyCode::OemQuestion },
	{ "Oemtilde", KeyCode::Oemtilde },
	{ "OemOpenBrackets", KeyCode::OemOpenBrackets },
	{ "OemPipe", KeyCode::OemPipe },
	{ "OemCloseBrackets", KeyCode::OemCloseBrackets },
	{ "OemQuotes", KeyCode::OemQuotes },
	{ "Oem8", KeyCode::Oem8 },
	{ "OemBackslash", KeyCode::OemBackslash },
	{ "ProcessKey", KeyCode::ProcessKey },
	{ "Attn", KeyCode::Attn },
	{ "Crsel", KeyCode::Crsel },
	{ "Exsel", KeyCode::Exsel },
	{ "EraseEof", KeyCode::EraseEof },
	{ "Play", KeyCode::Play },
	{ "Zoom", KeyCode::Zoom },
	{ "NoName", KeyCode::NoName },
	{ "Pa1", KeyCode::Pa1 },
	{ "OemClear", KeyCode::OemClear },
	{ "IMEAccept", KeyCode::IMEAccept },
	{ "Oem1", KeyCode::Oem1 },
	{ "Oem102", KeyCode::Oem102 },
	{ "Oem2", KeyCode::Oem2 },
	{ "Oem3", KeyCode::Oem3 },
	{ "Oem4", KeyCode::Oem4 },
	{ "Oem5", KeyCode::Oem5 },
	{ "Oem6", KeyCode::Oem6 },
	{ "Oem7", KeyCode::Oem7 },
	{ "Packet", KeyCode::Packet },
	{ "Sleep", KeyCode::Sleep }
};

BindingManager::BindingManager(console::Context* context)
{
	m_bindCommand = std::make_unique<ConsoleCommand>(context, "bind", [=] (const std::string& keyName, const std::string& commandString)
	{
		auto it = keyNameMap.find(keyName);

		if (it == keyNameMap.end())
		{
			console::Printf("Invalid key name %s\n", keyName.c_str());
			return;
		}

		m_bindings[it->second] = commandString;

		console::SetVariableModifiedFlags(ConVar_Archive);
	});

	m_unbindCommand = std::make_unique<ConsoleCommand>(context, "unbind", [=] (const std::string& keyName)
	{
		auto it = keyNameMap.find(keyName);

		if (it == keyNameMap.end())
		{
			console::Printf("Invalid key name %s\n", keyName.c_str());
			return;
		}

		m_bindings.erase(it->second);

		console::SetVariableModifiedFlags(ConVar_Archive);
	});

	m_unbindAllCommand = std::make_unique<ConsoleCommand>(context, "unbindall", [=] ()
	{
		m_bindings.clear();
	});

	m_keyListener = std::make_unique<EventListener<KeyEvent>>(std::bind(&BindingManager::HandleKeyEvent, this, std::placeholders::_1));
}

void BindingManager::HandleKeyEvent(const KeyEvent* ev)
{
	// find if this is in fact a mapped key
	auto it = m_bindings.find(ev->GetKeyCode());

	if (it == m_bindings.end())
	{
		return;
	}

	const std::string& commandString = it->second;

	// split the string by ';' (so button bindings get handled separately)
	int i = 0;
	std::stringstream thisCmd;

	bool hadButtonEvent = false;

	while (true)
	{
		if (i == commandString.length() || commandString[i] == ';')
		{
			std::string thisString = thisCmd.str();
			thisCmd.str("");

			// if this is a button binding
			if (thisString[0] == '+')
			{
				if (ev->IsDownEvent())
				{
					console::AddToBuffer(thisString + " " + std::to_string(static_cast<int>(ev->GetKeyCode())) + " " + std::to_string(ev->GetTime()) + "\n");
				}
				else
				{
					// up event is -[button cmd]
					console::AddToBuffer("-" + thisString.substr(1) + " " + std::to_string(static_cast<int>(ev->GetKeyCode())) + "\n");
				}

				hadButtonEvent = true;
			}
			else
			{
				// if not, just execute the command on down
				if (ev->IsDownEvent() || hadButtonEvent)
				{
					console::AddToBuffer(thisString + "\n");
				}
			}
		}

		if (i == commandString.length())
		{
			break;
		}

		thisCmd << commandString[i];

		i++;
	}
}

void BindingManager::WriteBindings(const TWriteLineCB& writeLineFunction)
{
	static std::map<KeyCode, std::string> reverseKeyMap;
	static std::once_flag reverseKeyInitFlag;

	// initialize a reverse mapping list
	if (reverseKeyMap.empty())
	{
		std::call_once(reverseKeyInitFlag, [] ()
		{
			for (const auto& entry : keyNameMap)
			{
				reverseKeyMap.insert({ entry.second, entry.first });
			}
		});
	}

	writeLineFunction("unbindall");

	for (auto& binding : m_bindings)
	{
		if (binding.second.find('"') != std::string::npos)
		{
			console::PrintWarning("Tried to write binding with \" character! (%s)\n", binding.second.c_str());
		}

		// TODO: handle bindings containing double quotes
		writeLineFunction("bind \"" + reverseKeyMap[binding.first] + "\" \"" + binding.second + "\"");
	}
}
}