#include "Controller.h"

Controller::Controller()
	: m_isPause(true), m_isControl(true),
	m_contolThread(), m_hotkey(), m_actionKey(0)
{
	m_contolThread = std::thread(&Controller::processControl, this);
}

Controller::~Controller()
{
	m_isControl.store(false);
	m_contolThread.join();
}

bool Controller::setPauseHotKey(std::string hotkey)
{

	if (!verifyKey(hotkey))
	{
		return false;
	}

	m_hotkey.clear();
	
	std::istringstream iss(hotkey);

	std::string key;
	while (std::getline(iss, key, '+')) 
	{
		const int virtualKey = getVirtualKeyCode(key);
		if (virtualKey) 
		{
			m_hotkey.push_back(virtualKey);
		}
	}

	return !m_hotkey.empty();
}

bool Controller::setActionKey(std::string key)
{
	if (!verifyKey(key))
	{
		return false;
	}

	const int virtualKey = getVirtualKeyCode(key);
	if (virtualKey)
	{
		m_actionKey = virtualKey;
		return true;
	}

	return false;
}

bool Controller::isPause() const
{
	return m_isPause.load();
}

void Controller::processControl()
{
	while (m_isControl)
	{
		if (checkPausePressed())
		{
			m_isPause.store(!m_isPause.load());
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
}

int Controller::getVirtualKeyCode(std::string key) const
{
	const std::unordered_map<std::string, int> validKeys =
	{
		{"F1", VK_F1}, {"F2", VK_F2}, {"F3", VK_F3},
		{"F4", VK_F4}, {"F5", VK_F5}, {"F6", VK_F6},
		{"F7", VK_F7}, {"F8", VK_F8}, {"F9", VK_F9},
		{"F10", VK_F10}, {"F11", VK_F11}, {"F12", VK_F12},

		{"ALT", VK_MENU}, {"CTRL", VK_CONTROL},
		{"SHIFT", VK_SHIFT}, {"WIN", VK_LWIN},

		{"A", 'A'}, {"B", 'B'}, {"C", 'C'}, {"D", 'D'},
		{"E", 'E'}, {"F", 'F'}, {"G", 'G'}, {"H", 'H'},
		{"I", 'I'}, {"J", 'J'}, {"K", 'K'}, {"L", 'L'},
		{"M", 'M'}, {"N", 'N'}, {"O", 'O'}, {"P", 'P'},
		{"Q", 'Q'}, {"R", 'R'}, {"S", 'S'}, {"T", 'T'},
		{"U", 'U'}, {"V", 'V'}, {"W", 'W'}, {"X", 'X'},
		{"Y", 'Y'}, {"Z", 'Z'}
	};

	const auto it = validKeys.find(key);
	return it != validKeys.end() ? it->second : 0;
}

bool Controller::checkPausePressed() const
{
	if (m_hotkey.empty())
	{
		return false;
	}

	for (int key : m_hotkey) 
	{
		if (!(GetAsyncKeyState(key) & KEY_MASK)) 
		{
			return false;
		}
	}

	return true;
}
