#ifndef __CONTROLLER_H
#define __CONTROLLER_H

#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include <unordered_map>

#include <Windows.h>

#include "utils.h"

#define KEY_MASK 0x8000

class Controller
{
public:
	explicit Controller();

	~Controller();

	bool setPauseHotKey(std::string hotkey);

	bool setActionKey(std::string key);

	bool isPause() const;

private:
	std::atomic<bool> m_isPause;

	std::atomic<bool> m_isControl;

	std::thread m_contolThread;

	std::vector<int> m_hotkey;

	int m_actionKey;

	void processControl();

	int getVirtualKeyCode(std::string key) const;

	bool checkPausePressed() const;
};

#endif // !__CONTROLLER_H
