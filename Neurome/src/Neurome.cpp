#include "Neurome.h"

std::atomic<bool> Neurome::m_running(true);

Neurome::Neurome()
	: m_code(ResultCode::Unknown),
	m_settings("osu!", true, 3000, "F6", 64, 64),
	m_process(), 
	m_memoryReader()
{
	setlocale(LC_ALL, "");
	system("cls");
	signal(SIGINT, sigintHandle);

	std::cout << "Press CTRL+C to exit" << std::endl;

	m_settings.init("data/settings.cfg");
	m_settings.print();

	if (!m_controller.setPauseHotKey(m_settings.pauseHotKey))
	{
		std::cerr << "ERR: Couldn't set the pause hotkey: " << m_settings.pauseHotKey << std::endl;
		m_code = ResultCode::KeyError;
		
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

		/*if (userConfig.isFullscreen)
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
		}*/

		if (!m_controller.setActionKey(userConfig.keyLeft))
		{
			std::cerr << "ERR: Couldn't set the action key: " << userConfig.keyLeft << std::endl;
			m_code = ResultCode::KeyError;

			return;
		}
		
		while (m_running && m_memoryReader.isAccessible())
		{
	/*		std::cout << "model status:\t" << (m_controller.isPause() ? "pause" : "active") << std::endl;
			std::cout << "osu! status:\t" << (m_memoryReader.isPlay() ? "play" : "menu") << std::endl;*/

			uint32_t sessionCount = 1;
			while (m_running && m_memoryReader.isPlay() && !m_controller.isPause())
			{
				clock_t start = clock();
				
				cv::Mat frame;
				if (!m_process.getCaptureWindow(&frame, m_settings.inputWidth, m_settings.inputHeight))
				{
					std::cerr << "ERR: The captured window could not be read" << std::endl;
					m_code = ResultCode::CaptureWindowError;

					return;
				}
				std::cout << "frame preprocessing time: " << double(clock() - start) << "ms" << std::endl;

				cv::imshow("Window Capture", frame);
				cv::waitKey(27);


				//const std::string header = "Session #" + std::to_string(sessionCount);
				//const uint32_t padding = (lineWidth - header.length()) / 2;

				//std::cout << makeLine(padding, lineSymbol) << header << makeLine(padding, lineSymbol) << std::endl;
				//std::cout << "perfect:" << std::setw(8) << m_memoryReader.getHitPerfect() << std::endl;
				//std::cout << "300:" << std::setw(8) << m_memoryReader.getHit300() << std::endl;
				//std::cout << "200:" << std::setw(8) << m_memoryReader.getHit200() << std::endl;
				//std::cout << "100:" << std::setw(8) << m_memoryReader.getHit100() << std::endl;
				//std::cout << "50:" << std::setw(8) << m_memoryReader.getHit50() << std::endl;
				//std::cout << "miss:" << std::setw(8) << m_memoryReader.getHitMiss() << std::endl;
				//std::cout << makeLine(lineWidth, lineSymbol) << std::endl;
			}

			++sessionCount;
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

Neurome::Settings_t::Settings(std::string clientName, bool isAwaitProcess,
							  uint32_t awaitProcessDelay, std::string pauseHotKey,
							  uint32_t inputWidth, uint32_t inputHeight)
	: clientName(clientName), isAwaitProcess(isAwaitProcess),
	awaitProcessDelay(awaitProcessDelay), pauseHotKey(pauseHotKey),
	inputWidth(inputWidth), inputHeight(inputHeight) {}

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

	const std::string clientName = (*configHandler)["clientName"];
	if (!clientName.empty())
	{
		this->clientName = clientName;
	}
	else
	{
		(*configHandler)["clientName"] = this->clientName;
	}

	const std::string isAwaitProcessStr = toLower((*configHandler)["isAwaitProcess"]);
	if (!isAwaitProcessStr.empty())
	{
		this->isAwaitProcess = isAwaitProcessStr == "true";
	}
	else
	{
		(*configHandler)["isAwaitProcess"] = boolToString(this->isAwaitProcess);
	}

	const std::string awaitProcessDelayStr = (*configHandler)["awaitProcessDelay"];
	if (isPositiveInteger(awaitProcessDelayStr))
	{
		awaitProcessDelay = std::stoi(awaitProcessDelayStr);
	}
	else
	{
		(*configHandler)["awaitProcessDelay"] = std::to_string(awaitProcessDelay);
	}

	const std::string pauseHotKey = (*configHandler)["pauseHotKey"];
	if (verifyKey(pauseHotKey))
	{
		this->pauseHotKey = pauseHotKey;
	}
	else
	{
		(*configHandler)["pauseHotKey"] = this->pauseHotKey;
	}

	const std::string inputWidthStr = (*configHandler)["inputWidth"];
	if (isPositiveInteger(inputWidthStr))
	{
		inputWidth = std::stoi(inputWidthStr);
	}
	else
	{
		(*configHandler)["inputWidth"] = std::to_string(inputWidth);
	}

	const std::string inputHeightStr = (*configHandler)["inputHeight"];
	if (isPositiveInteger(inputHeightStr))
	{
		inputHeight = std::stoi(inputHeightStr);
	}
	else
	{
		(*configHandler)["inputHeight"] = std::to_string(inputHeight);
	}

	return ResultCode::Success;
}

void Neurome::Settings_t::print() const
{
	std::cout << "The following settings are loaded:" << std::endl;
	std::cout << " - clientName: " << clientName << std::endl;
	std::cout << " - isAwaitProcess: " << isAwaitProcess << std::endl;
	std::cout << " - awaitProcessDelay: " << awaitProcessDelay << std::endl;
	std::cout << " - pauseHotKey: " << pauseHotKey << std::endl;
	std::cout << " - inputWidth: " << inputWidth << std::endl;
	std::cout << " - inputHeight: " << inputHeight << std::endl;
}

Neurome::UserConfig_t::UserConfig()
	: keyLeft(""), isFullscreen(false) {}