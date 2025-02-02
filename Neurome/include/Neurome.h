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
#include "ProximalPolicyOptimization.h"
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
		DeviceError,
		AgentInitializeError
	};

	typedef struct Settings
	{
		std::string clientName;
		bool isAwaitProcess;
		uint32_t awaitProcessDelay;
		std::string pauseHotKey;
		std::string modelPath;
		uint32_t inputWidth;
		uint32_t inputHeight;
		uint32_t hiddenSize;
		uint32_t epochs;
		uint32_t bufferSize;
		float actorLr;
		float criticLr;
		float gamma;
		float epsilon;

		explicit Settings(std::string clientName, bool isAwaitProcess,
						  uint32_t awaitProcessDelay, std::string pauseHotKey, std::string modelPath,
						  uint32_t inputWidth, uint32_t inputHeight,
			              uint32_t hiddenSize, uint32_t epochs, uint32_t bufferSize,
						  float actorLr, float criticLr, float gamma, float epsilon);

		void init(std::string settingsPath);

		ResultCode merge(ConfigHandler *configHandler);

		void print() const;

		void parseStr(ConfigHandler *configHandler, std::string *value, std::string field);

		void parseUInt(ConfigHandler *configHandler, uint32_t *value, std::string field);

		void parseFloat(ConfigHandler *configHandler, float *value, std::string field);

		void parseBool(ConfigHandler *configHandler, bool *value, std::string field);

		void parseKey(ConfigHandler *configHandler, std::string *value, std::string field);

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

	ProximalPolicyOptimization m_ppo;
};
#endif // !__NEUROME_H
