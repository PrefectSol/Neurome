<<<<<<< HEAD
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
#include <sddl.h>
#include <csignal>

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

	bool IsAdmin();

	bool GetAdmin();

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
		float rewardPerfect;
		float reward300;
		float reward200;
		float reward100;
		float reward50;
		float rewardMiss;
		float lambda;
		float gradClip;
		float movementNoise;
		float actionNoise;
		float anyHitReward;
		float hitStreakReward;
		float targetMovement;
		float sigmaMovement;

		explicit Settings(std::string clientName, bool isAwaitProcess,
						  uint32_t awaitProcessDelay, std::string pauseHotKey, std::string modelPath,
						  uint32_t inputWidth, uint32_t inputHeight,
			              uint32_t hiddenSize, uint32_t epochs, uint32_t bufferSize,
						  float actorLr, float criticLr, float gamma, float epsilon,
						  float rewardPerfect, float reward300, float reward200,
						  float reward100, float reward50, float rewardMiss,
						  float lambda, float gradClip,
						  float movementNoise, float actionNoise,
						  float anyHitReward, float hitStreakReward, float targetMovement, float sigmaMovement);

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

	float execute(const torch::Tensor &actions, uint32_t width, uint32_t height);

	ResultCode m_code;

	Settings_t m_settings;

	ProcessHandler m_process;

	MemoryReader m_memoryReader;

	Controller m_controller;

	ProximalPolicyOptimization m_ppo;

	const float m_confidenceThreshold;

	bool m_isHold;

	float m_hitStreak;

	float m_lastDx;

	float m_lastDy;
};
#endif // !__NEUROME_H
=======
#ifndef __NEUROME_H
#define __NEUROME_H

#include <cstdint>

#include "ConfigHandler.h"
#include "ProcessHandler.h"
#include "utils.h"

#define NONE 0
#define OK_MASK 1 << 0
#define BLOCK_TRAFFIC_MASK 1 << 1
#define READ_USER_CONFIG_MASK 1 << 2
#define RESTART_MASK 1 << 3

class Neurome
{
public:
	explicit Neurome() noexcept;

	~Neurome();

private:
	typedef struct Settings
	{
	public:
		std::string clientName;

		void load(std::string path);

		void save(std::string path) const;

	private:
		bool merge(ConfigHandler *configHandler);

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

		bool read(std::string path);
	} UserConfig_t;

	std::string getCfgPath() const;

	void toWindowedMode() const;

	ProcessHandler m_process;

protected:
	std::string m_settingsPath;

	Settings_t m_settings;

	void initSettings();

	std::string getProcessPath() const;

	int32_t attachProcess();

	int32_t detachProcess();
};
#endif // !__NEUROME_H
>>>>>>> 73860c5e4f984f52cafff7d349a394f3bb10aeba
