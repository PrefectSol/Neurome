#ifndef __NEUROME_H
#define __NEUROME_H

#include <iostream>
#include <string>
#include <csignal>
#include <atomic>
#include <locale>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>

#include <Windows.h>

#include "ConfigHandler.h"
#include "ProcessHandler.h"
#include "MemoryReader.h"
#include "Controller.h"
#include "utils.h"

class Neurome
{
public:
	explicit Neurome();

	~Neurome();

	void start();

	int exit() const;

private:
	enum ResultCode
	{
		Unknown = -1,
		Success = 0,
		Interrupted, 
		NullObject,
		KeyError,
		ReadConfigError,
		NotAwait,
		GetWindowError,
		CaptureWindowError,

	};

	typedef struct Settings
	{
		std::string clientName;
		bool isAwaitProcess;
		uint32_t awaitProcessDelay;
		std::string pauseHotKey;
		uint32_t inputWidth;
		uint32_t inputHeight;

		explicit Settings(std::string clientName, bool isAwaitProcess,
						  uint32_t awaitProcessDelay, std::string pauseHotKey,
						  uint32_t inputWidth, uint32_t inputHeight);

		void init(std::string settingsPath);

		ResultCode merge(ConfigHandler *configHandler);

		void print() const;

	} Settings_t;

	typedef struct UserConfig
	{
		std::string keyLeft;
		bool isFullscreen;

		explicit UserConfig();
	} UserConfig_t;
	
	static void sigintHandle(int signal);

	static std::atomic<bool> m_running;

	bool openProcess();

	std::string getCfgPath() const;

	bool readUserConfig(UserConfig_t *userConfig) const;

	void toWindowedMode() const;

	ResultCode m_code;

	Settings_t m_settings;

	ProcessHandler m_process;

	MemoryReader m_memoryReader;

	Controller m_controller;
};
#endif // !__NEUROME_H
