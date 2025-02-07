#include "Neurome.h"

std::atomic<bool> Neurome::m_running(true);

Neurome::Neurome()
	: m_code(ResultCode::Unknown),
	m_settings("osu!", true, 3000, "F6", "models/agent", 64, 64,
		       512, 3, 32, 3e-4f, 3e-4f, 0.99f, 0.2f,
			   1.0f, 0.8f, 0.6f, 0.4f, 0.2f, -0.5f),
	m_process(), 
	m_memoryReader(),
	m_controller(),
	m_ppo(),
	m_confidenceThreshold(0.75f),
	m_isHold(false)
{
	setlocale(LC_ALL, "");
	signal(SIGINT, sigintHandle);

	std::cout << "Press CTRL+C to exit" << std::endl;

	// Add check admin 
	// if don`t have - print warn

	m_settings.init("data/settings.cfg");
	m_settings.print();

	if (!m_controller.setPauseHotKey(m_settings.pauseHotKey))
	{
		std::cerr << "ERR: Couldn't set the pause hotkey: " << m_settings.pauseHotKey << std::endl;
		m_code = ResultCode::KeyError;
		
		return;
	}

	LoadLibraryA("torch_cuda.dll");
	if (!torch::cuda::is_available())
	{
		std::cerr << "ERR: CUDA is unavailable - Couldn't load `torch_cuda.dll`. CPU is not supported" << std::endl;
		m_code = ResultCode::DeviceError;

		return;
	}

	if (!m_ppo.init(m_settings.modelPath, m_settings.hiddenSize, m_settings.gamma, m_settings.epsilon,
		m_settings.epochs, m_settings.bufferSize, m_settings.actorLr, m_settings.criticLr))
	{
		std::cerr << "ERR: Couldn't initialize the model correctly" << std::endl;
		m_code = ResultCode::AgentInitializeError;

		return;
	}

	m_code = ResultCode::Success;
}

Neurome::~Neurome() {}

void Neurome::start()
{
	if (m_code != ResultCode::Success)
	{
		return;
	}

	const uint32_t lineWidth = 40;
	const char lineSymbol = '-';

	while (m_running)
	{
		if (!openProcess())
		{
			return;
		}

		UserConfig_t userConfig;
		if (!readUserConfig(&userConfig))
		{
			std::cerr << "ERR: Couldn't read the osu!.{USER}.cfg file correctly" << std::endl;
			m_code = ResultCode::ReadConfigError;

			return;
		}

		if (userConfig.isFullscreen)
		{
			std::cerr << "WARN: The application is open in full-screen mode. Switching to windowed mode..." << std::endl;
			
			toWindowedMode();
			if (!m_process.restart())
			{
				std::cerr << "WARN: The process could not be restarted. Please restart the process manually within the next 10 seconds" << std::endl;
				std::this_thread::sleep_for(std::chrono::seconds(8));
			}

			std::this_thread::sleep_for(std::chrono::seconds(2));

			continue;
		}

		if (!m_controller.setActionKey(userConfig.keyLeft))
		{
			std::cerr << "ERR: Couldn't set the action key: " << userConfig.keyLeft << std::endl;
			m_code = ResultCode::KeyError;

			return;
		}
		
		while (m_running && m_memoryReader.isAccessible())
		{
			while (m_running && m_memoryReader.isPlay() && !m_controller.isPause())
			{
				uint32_t width, height;
				if (!m_process.getWindowSizes(&width, &height))
				{
					std::cerr << "WARN: The window is minimized. I can't see anything :(" << std::endl;
					continue;
				}

				clock_t start = clock();
				torch::Tensor frame;
				if (!m_process.getCaptureWindow(&frame, m_settings.inputWidth, m_settings.inputHeight))
				{
					std::cerr << "ERR: The captured window could not be read" << std::endl;
					m_code = ResultCode::CaptureWindowError;

					return;
				}
				const clock_t preprocessingTime = clock() - start;

				start = clock();
				const torch::Tensor prob = m_ppo.inference(frame).squeeze();
				const clock_t inferenceTime = clock() - start;

				const float reward = execute(prob.to(torch::kCPU), width, height);
				const bool done = !m_running || !m_memoryReader.isPlay() || m_controller.isPause();
				
				start = clock();
				torch::Tensor nextFrame;
				if (!m_process.getCaptureWindow(&nextFrame, m_settings.inputWidth, m_settings.inputHeight))
				{
					std::cerr << "ERR: The captured window could not be read" << std::endl;
					m_code = ResultCode::CaptureWindowError;

					return;
				}
				const clock_t nextPreprocessingTime = clock() - start;

				start = clock();
				m_ppo.storeExperience(frame, prob, reward, nextFrame, done ? 1.0f : 0.0f, prob);
				const clock_t expTime = clock() - start;

				std::cout << makeLine(lineWidth, lineSymbol) << std::endl;
				std::cout << "frame preprocessing time: " << preprocessingTime << "ms" << std::endl;
				std::cout << "model inference time: " << inferenceTime << "ms" << std::endl;
				std::cout << "next frame preprocessing time: " << nextPreprocessingTime << "ms" << std::endl;
				std::cout << "store experience time: " << expTime << "ms" << std::endl;
				std::cout << "total time: " << preprocessingTime + inferenceTime + nextPreprocessingTime + expTime << "ms" << std::endl;
				std::cout << std::endl;
				std::cout << "perfect:" << std::setw(8) << m_memoryReader.getHitPerfect() << std::endl;
				std::cout << "300:" << std::setw(8) << m_memoryReader.getHit300() << std::endl;
				std::cout << "200:" << std::setw(8) << m_memoryReader.getHit200() << std::endl;
				std::cout << "100:" << std::setw(8) << m_memoryReader.getHit100() << std::endl;
				std::cout << "50:" << std::setw(8) << m_memoryReader.getHit50() << std::endl;
				std::cout << "miss:" << std::setw(8) << m_memoryReader.getHitMiss() << std::endl;
				std::cout << std::endl;
				std::cout << "reward: " << reward << std::endl;
				std::cout << "is done: " << done << std::endl;
				std::cout << makeLine(lineWidth, lineSymbol) << std::endl;
			
				std::cout << "model status:\t" << (m_controller.isPause() ? "pause" : "active") << std::endl;
				std::cout << "osu! status:\t" << (m_memoryReader.isPlay() ? "play" : "menu") << std::endl;
			}

			//std::cout << "model status:\t" << (m_controller.isPause() ? "pause" : "active") << std::endl;
			//std::cout << "osu! status:\t" << (m_memoryReader.isPlay() ? "play" : "menu") << std::endl;
		
			if (m_isHold)
			{
				m_controller.releaseKey();
				m_isHold = false;
			}
		}

		std::this_thread::sleep_for(std::chrono::seconds(2));
	}
}

int Neurome::exit() const
{
	if (!m_running)
	{
		return ResultCode::Interrupted;
	}

	return m_code;
}

void Neurome::sigintHandle(int signal)
{
	m_running = false;
}

bool Neurome::openProcess()
{
	if (!m_process.getProcess(m_settings.clientName))
	{
		std::cerr << "WARN: Couldn't read PID and HWND of the client process: " << m_settings.clientName << std::endl;

		if (!m_settings.isAwaitProcess)
		{
			m_code = ResultCode::NotAwait;
			return false;
		}

		std::cout << "Waiting for the process to open..." << std::endl;

		bool *loop = reinterpret_cast<bool*>(&Neurome::m_running);
		if (!m_process.getProcess(m_settings.clientName, loop, m_settings.awaitProcessDelay))
		{
			std::cerr << "ERR: The process does not have a graphical window" << std::endl;
			m_code = ResultCode::GetWindowError;

			return false;
		};

		if (!m_running)
		{
			m_code = ResultCode::Interrupted;
			return false;
		}
	}

	std::cout << "Found the process: " << m_process.getProcessPath() << std::endl;

	if (!m_process.blockTraffic())
	{
		std::cerr << "WARN: The process's network traffic could not be blocked, and the threat risk increaseds" << std::endl;
	}

	return true;
}

std::string Neurome::getCfgPath() const
{
	const std::filesystem::path exePath(m_process.getProcessPath());

	DWORD usernameLen = MAX_PATH;
	wchar_t username[MAX_PATH];
	GetUserName(username, &usernameLen);

	const std::filesystem::path cfgPath = exePath.parent_path() / ("osu!." + std::string(username, username + usernameLen - 1) + ".cfg");

	return cfgPath.string();
}

bool Neurome::readUserConfig(UserConfig_t *userConfig) const
{
	if (!userConfig)
	{
		return false;
	}

	ConfigHandler osuConfig(getCfgPath());

	const std::string keyLeft = osuConfig["keyOsuLeft"];
	if (keyLeft.empty())
	{
		return false;
	}

	const std::string isFullscreenStr = osuConfig["Fullscreen"];
	if (isFullscreenStr != "0" && isFullscreenStr != "1")
	{
		return false;
	}

	userConfig->keyLeft = keyLeft;
	userConfig->isFullscreen = isFullscreenStr == "1";

	return true;
}

void Neurome::toWindowedMode() const
{
	ConfigHandler osuConfig(getCfgPath());
	osuConfig["Fullscreen"] = "0"; 

	osuConfig.save();
}

float Neurome::execute(const torch::Tensor &actions, uint32_t width, uint32_t height)
{
	if (actions.sizes() != torch::IntArrayRef({4}) || width == 0 || height == 0)
	{
		return 0.0f;
	}

	uint32_t offsetX = 0;
	uint32_t offsetY = 0;
	m_process.getWindowOffset(&offsetY, &offsetX);

	uint32_t lastX, lastY;
	m_controller.getCursorPos(&lastX, &lastY);

	const std::array<int32_t, 6> lastHits = m_memoryReader.getHits();

	const auto accessor = actions.accessor<float, 1>();
	
	const float x = accessor[0] * width;
	const float y = accessor[1] * height;
	const float clickConfidence = accessor[2];
	const float holdConfidence = accessor[3];

	m_controller.mouseTo(offsetX + x, offsetY + y);

	if (clickConfidence >= m_confidenceThreshold)
	{
		m_controller.clickKey();
	}
	else if (!m_isHold && holdConfidence >= m_confidenceThreshold)
	{
		m_controller.pressKey();
		m_isHold = true;
	}
	else if (m_isHold && holdConfidence < m_confidenceThreshold)
	{
		m_controller.releaseKey();
		m_isHold = false;
	}

	const float positionDelta = std::abs(x - lastX) + std::abs(y - lastY);
	const float movementReward = std::sin(positionDelta * M_PI / width) * 0.5f;

	std::array<int32_t, 6> hits = m_memoryReader.getHits();

	float hitReward = 0.0f;
	if (hits != lastHits)
	{
		hitReward += (hits[0] - lastHits[0]) * m_settings.rewardPerfect;
		hitReward += (hits[1] - lastHits[1]) * m_settings.reward300;
		hitReward += (hits[2] - lastHits[2]) * m_settings.reward200;
		hitReward += (hits[3] - lastHits[3]) * m_settings.reward100;
		hitReward += (hits[4] - lastHits[4]) * m_settings.reward50;
		hitReward += (hits[5] - lastHits[5]) * m_settings.rewardMiss;
	}
	
	return movementReward + hitReward;
}

Neurome::Settings_t::Settings(std::string clientName, bool isAwaitProcess,
							  uint32_t awaitProcessDelay, std::string pauseHotKey, std::string modelPath,
							  uint32_t inputWidth, uint32_t inputHeight,
						 	  uint32_t hiddenSize, uint32_t epochs, uint32_t bufferSize,
					 	      float actorLr, float criticLr, float gamma, float epsilon,
						  	  float rewardPerfect, float reward300, float reward200,
							  float reward100, float reward50, float rewardMiss)
	: clientName(clientName), isAwaitProcess(isAwaitProcess),
	awaitProcessDelay(awaitProcessDelay), pauseHotKey(pauseHotKey), modelPath(modelPath),
	inputWidth(inputWidth), inputHeight(inputHeight),
	hiddenSize(hiddenSize), epochs(epochs), bufferSize(bufferSize),
    actorLr(actorLr), criticLr(criticLr), gamma(gamma), epsilon(epsilon),
	rewardPerfect(rewardPerfect), reward300(reward300), reward200(reward200), 
	reward100(reward100), reward50(reward50), rewardMiss(rewardMiss) {}

void Neurome::Settings_t::init(std::string settingsPath)
{
	ConfigHandler configHandler(settingsPath);
	if (!configHandler.isGood())
	{
		std::cerr << "WARN: Couldn't read the settings file: " << settingsPath << std::endl;
	}

	const ResultCode mergeCode = merge(&configHandler);
	if (mergeCode != ResultCode::Success)
	{
		std::cerr << "WARN (" << mergeCode << "): I have no idea when this error is even possible" << std::endl;
	}

	if (!configHandler.save())
	{
		std::cerr << "WARN: Couldn't fix the settings file: " << settingsPath << std::endl;
	}
}

Neurome::ResultCode Neurome::Settings_t::merge(ConfigHandler *configHandler)
{
	if (!configHandler)
	{
		return ResultCode::NullObject;
	}

	parseStr(configHandler, &this->clientName, "clientName");
	parseBool(configHandler, &this->isAwaitProcess, "isAwaitProcess");
	parseUInt(configHandler, &this->awaitProcessDelay, "awaitProcessDelay");
	parseKey(configHandler, &this->pauseHotKey, "pauseHotKey");
	parseStr(configHandler, &this->modelPath, "modelPath");
	parseUInt(configHandler, &this->inputWidth, "inputWidth");
	parseUInt(configHandler, &this->inputHeight, "inputHeight");
	parseUInt(configHandler, &this->hiddenSize, "hiddenSize");
	parseUInt(configHandler, &this->epochs, "epochs");
	parseUInt(configHandler, &this->bufferSize, "bufferSize");
	parseFloat(configHandler, &this->actorLr, "actorLr");
	parseFloat(configHandler, &this->criticLr, "criticLr");
	parseFloat(configHandler, &this->gamma, "gamma");
	parseFloat(configHandler, &this->epsilon, "epsilon");
	parseFloat(configHandler, &this->rewardPerfect, "rewardPerfect");
	parseFloat(configHandler, &this->reward300, "reward300");
	parseFloat(configHandler, &this->reward200, "reward200");
	parseFloat(configHandler, &this->reward100, "reward100");
	parseFloat(configHandler, &this->reward50, "reward50");
	parseFloat(configHandler, &this->rewardMiss, "rewardMiss");

	return ResultCode::Success;
}

void Neurome::Settings_t::print() const
{
	std::cout << "The following settings are loaded:" << std::endl;
	std::cout << " - clientName: " << clientName << std::endl;
	std::cout << " - isAwaitProcess: " << isAwaitProcess << std::endl;
	std::cout << " - awaitProcessDelay: " << awaitProcessDelay << std::endl;
	std::cout << " - pauseHotKey: " << pauseHotKey << std::endl;
	std::cout << " - modelPath: " << modelPath << std::endl;
	std::cout << " - inputWidth: " << inputWidth << std::endl;
	std::cout << " - inputHeight: " << inputHeight << std::endl;
	std::cout << " - hiddenSize: " << hiddenSize << std::endl;
	std::cout << " - epochs: " << epochs << std::endl;
	std::cout << " - actorLr: " << actorLr << std::endl;
	std::cout << " - criticLr: " << criticLr << std::endl;
	std::cout << " - gamma: " << gamma << std::endl;
	std::cout << " - epsilon: " << epsilon << std::endl;
	std::cout << " - rewardPerfect: " << rewardPerfect << std::endl;
	std::cout << " - reward300: " << reward300 << std::endl;
	std::cout << " - reward200: " << reward200 << std::endl;
	std::cout << " - reward100: " << reward100 << std::endl;
	std::cout << " - reward50: " << reward50 << std::endl;
	std::cout << " - rewardMiss: " << rewardMiss << std::endl;
}

void Neurome::Settings_t::parseStr(ConfigHandler *configHandler, std::string *value, std::string field)
{
	const std::string str = (*configHandler)[field];
	if (!str.empty())
	{
		*value = str;
		return;
	}

	(*configHandler)[field] = *value;
}

void Neurome::Settings_t::parseUInt(ConfigHandler *configHandler, uint32_t *value, std::string field)
{
	const std::string str = (*configHandler)[field];
	if (isPositiveInteger(str))
	{
		*value = std::stoi(str);
		return;
	}

	(*configHandler)[field] = std::to_string(*value);
}

void Neurome::Settings_t::parseFloat(ConfigHandler *configHandler, float *value, std::string field)
{
	const std::string str = (*configHandler)[field];
	if (isFloat(str))
	{
		*value = std::stof(str);
		return;
	}

	(*configHandler)[field] = std::to_string(*value);
}

void Neurome::Settings_t::parseBool(ConfigHandler *configHandler, bool *value, std::string field)
{
	const std::string str = toLower((*configHandler)[field]);
	if (!str.empty())
	{
		*value = str == "true";
		return;
	}

	(*configHandler)[field] = boolToString(*value);
}

void Neurome::Settings_t::parseKey(ConfigHandler *configHandler, std::string *value, std::string field)
{
	const std::string key = (*configHandler)[field];
	if (verifyKey(key))
	{
		*value = key;
		return;
	}

	(*configHandler)[field] = *value;
}

Neurome::UserConfig_t::UserConfig()
	: keyLeft(""), isFullscreen(false) {}


