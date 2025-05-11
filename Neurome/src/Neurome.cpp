<<<<<<< HEAD
#include "Neurome.h"

std::atomic<bool> Neurome::m_running(true);

Neurome::Neurome()
	: m_code(ResultCode::Unknown),
	m_settings("osu!", true, 3000, "F6", "models/agent", 128, 128,
		       512, 3, 2048, 3e-4f, 1e-3f, 0.99f, 0.2f,
			   1.0f, 0.8f, 0.6f, 0.4f, 0.2f, -0.5f, 0.95f, 0.5f, 0.1f, 0.1f,
			   0.1f, 0.1f, 5.0f, 2.0f),
	m_process(), 
	m_memoryReader(),
	m_controller(),
	m_ppo(),
	m_confidenceThreshold(0.75f),
	m_isHold(false),
	m_hitStreak(0.0f),
	m_lastDx(0.0f), m_lastDy(0.0f)
{
	setlocale(LC_ALL, "");
	signal(SIGINT, sigintHandle);

	GetAdmin();

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

	if (!m_ppo.init(m_settings.modelPath, m_settings.hiddenSize, m_settings.inputWidth, m_settings.inputHeight,
					m_settings.gamma, m_settings.epsilon, m_settings.lambda, m_settings.gradClip, 
					m_settings.movementNoise, m_settings.actionNoise,
					m_settings.epochs, m_settings.bufferSize, m_settings.actorLr, m_settings.criticLr))
	{
		std::cerr << "ERR: Couldn't initialize the model correctly" << std::endl;
		m_code = ResultCode::AgentInitializeError;

		return;
	}

	if (!m_ppo.load())
	{
		std::cerr << "WARN: Couldn't load pre-trained model" << std::endl;
	}

	m_code = ResultCode::Success;
}

Neurome::~Neurome() {}

bool Neurome::GetAdmin() {

	if (IsAdmin()) 
	{
		return true;
	}
	wchar_t path[MAX_PATH];
	GetModuleFileName(NULL, path, MAX_PATH);

	SHELLEXECUTEINFO SEI = { 0 };
	DWORD NeuromePID = GetCurrentProcessId();

	SEI.cbSize = sizeof(SHELLEXECUTEINFO);
	SEI.lpVerb = L"runas";
	SEI.lpFile = path;
	SEI.nShow = SW_SHOWNORMAL;

	if (!ShellExecuteEx(&SEI))
	{
		CloseHandle(SEI.hProcess);

		return false;
	}
	else
	{
		HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, NeuromePID);
		if (hProcess != NULL)
		{
			TerminateProcess(hProcess, 0);
			CloseHandle(hProcess);
		}
	}

	CloseHandle(SEI.hProcess);

	return false;
}

bool Neurome::IsAdmin() {

	HANDLE NeuromeToken = NULL;
	PSID AdminGroupSID(nullptr);

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &NeuromeToken))
	{
		CloseHandle(NeuromeToken);
		return false;
	}

	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
	AllocateAndInitializeSid(&NtAuthority,
	 						 2,
	 						 SECURITY_BUILTIN_DOMAIN_RID,
	 						 DOMAIN_ALIAS_RID_ADMINS,
	 						 NULL,
	 						 NULL,
	 						 NULL,
	 						 NULL,
	 						 NULL,
	 						 NULL,
	 						 &AdminGroupSID);

	TOKEN_ELEVATION NeuromeElevation;
	PTOKEN_ELEVATION TokenInfo(nullptr);
	DWORD TokenIS = sizeof(TOKEN_ELEVATION);

	GetTokenInformation(NeuromeToken, TokenElevation, NULL, 0, &TokenIS);

	TokenInfo = (PTOKEN_ELEVATION)malloc(TokenIS);

	if (!GetTokenInformation(NeuromeToken, TokenElevation, &NeuromeElevation, TokenIS, &TokenIS))
	{
		std::cout << "Can't get token info. Err: " << GetLastError();

		free(TokenInfo);
		TokenInfo = nullptr;
		CloseHandle(NeuromeToken);
		FreeSid(AdminGroupSID);
		AdminGroupSID = nullptr; 

		return false;
	}

	if (NeuromeElevation.TokenIsElevated)
	{
		free(TokenInfo);
		TokenInfo = nullptr;
		CloseHandle(NeuromeToken);
		FreeSid(AdminGroupSID);
		AdminGroupSID = nullptr;

		return true;
	}

		free(TokenInfo);
		TokenInfo = nullptr;
		CloseHandle(NeuromeToken);
		FreeSid(AdminGroupSID);
		AdminGroupSID = nullptr;

		return false;
}

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
				//std::cout << "frame preprocessing time: " << preprocessingTime << "ms" << std::endl;
				//std::cout << "model inference time: " << inferenceTime << "ms" << std::endl;
				//std::cout << "next frame preprocessing time: " << nextPreprocessingTime << "ms" << std::endl;
				//std::cout << "store experience time: " << expTime << "ms" << std::endl;
				//std::cout << "total time: " << preprocessingTime + inferenceTime + nextPreprocessingTime + expTime << "ms" << std::endl;
				//std::cout << std::endl;
				//std::cout << "perfect:" << std::setw(8) << m_memoryReader.getHitPerfect() << std::endl;
				//std::cout << "300:" << std::setw(8) << m_memoryReader.getHit300() << std::endl;
				//std::cout << "200:" << std::setw(8) << m_memoryReader.getHit200() << std::endl;
				//std::cout << "100:" << std::setw(8) << m_memoryReader.getHit100() << std::endl;
				//std::cout << "50:" << std::setw(8) << m_memoryReader.getHit50() << std::endl;
				//std::cout << "miss:" << std::setw(8) << m_memoryReader.getHitMiss() << std::endl;
				std::cout << std::endl;
				std::cout << "reward: " << reward << std::endl;
				std::cout << "is done: " << done << std::endl;
				std::cout << "actor loss: " << m_ppo.getActorLoss() << std::endl;
				std::cout << "critic loss: " << m_ppo.getCriticLoss() << std::endl;
				std::cout << makeLine(lineWidth, lineSymbol) << std::endl;
			
				//std::cout << "model status:\t" << (m_controller.isPause() ? "pause" : "active") << std::endl;
				//std::cout << "osu! status:\t" << (m_memoryReader.isPlay() ? "play" : "menu") << std::endl;
			}

			//std::cout << "model status:\t" << (m_controller.isPause() ? "pause" : "active") << std::endl;
			//std::cout << "osu! status:\t" << (m_memoryReader.isPlay() ? "play" : "menu") << std::endl;
		
			if (m_isHold)
			{
				m_controller.releaseKey();
				m_isHold = false;
			}

			m_lastDx = 0.0f;
			m_lastDy = 0.0f;

			m_ppo.save();
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
	if (signal == WM_CLOSE || signal == SIGINT || signal == SIGTERM)
	{
		m_running = false;
	}
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
	if (actions.sizes() != torch::IntArrayRef({3}) || width == 0 || height == 0)
	{
		return 0.0f;
	}

	uint32_t lastX, lastY;
	m_controller.getCursorPos(&lastX, &lastY);

	const std::array<int32_t, 6> lastHits = m_memoryReader.getHits();

	const auto accessor = actions.accessor<float, 1>();
	
	const float dx = accessor[0];
	const float dy = accessor[1];
	const float confidence = accessor[2];

	uint32_t posX = lastX + dx * 50;
	uint32_t posY = lastY + dy * 50;

	std::cout << "dx: " << dx << std::endl;
	std::cout << "dy: " << dy << std::endl;
	std::cout << "posX: " << posX << std::endl;
	std::cout << "posY: " << posY << std::endl;
	float boundary_penalty = 0.0f;
	uint32_t top, left, bottom, right;
	if (m_process.getWindowOffsets(&top, &left, &bottom, &right)) {
		// Ðàññ÷èòûâàåì ðàññòîÿíèå âûõîäà çà êàæäóþ ãðàíèöó
		float dist_left = std::max(0.0f, float(left - posX));
		float dist_right = std::max(0.0f, float(posX - right));
		float dist_top = std::max(0.0f, float(top - posY));
		float dist_bottom = std::max(0.0f, float(posY - bottom));

		// Ñóììàðíîå ðàññòîÿíèå âûõîäà çà ãðàíèöû
		float total_dist = dist_left + dist_right + dist_top + dist_bottom;

		// Øòðàô ïðîïîðöèîíàëåí ðàññòîÿíèþ âûõîäà
		boundary_penalty = total_dist > 0 ? -std::min(1.0f, total_dist) : 0.0f;

		// Îãðàíè÷èâàåì ïîçèöèþ ìûøè ãðàíèöàìè îêíà
		posX = std::clamp(posX, left, right);
		posY = std::clamp(posY, top, bottom);
	}

	m_controller.mouseTo(posX, posY);

	//if (!m_isHold && confidence >= 0.5f)
	//{
	//	m_controller.pressKey();
	//	m_isHold = true;
	//}
	//else if (m_isHold && confidence < 0.5f)
	//{
	//	m_controller.releaseKey();
	//	m_isHold = false;
	//}

	float movement_reward = std::sqrt(dx * dx + dy * dy);
	float smoothness_penalty = -std::pow(std::sqrt(dx * dx + dy * dy), 2);

	float direction_change = std::abs(dx - m_lastDx) + std::abs(dy - m_lastDy);
	float smoothness_reward = -direction_change;

	m_lastDx = dx;
	m_lastDy = dy;

	const float movementReward = movement_reward + smoothness_penalty + smoothness_reward + boundary_penalty;

	std::array<int32_t, 6> hits = m_memoryReader.getHits();

	float hitReward = 0.0f;
	//if (hits != lastHits)
	//{
	//	hitReward += float(hits[0] - lastHits[0]) * m_settings.rewardPerfect;
	//	hitReward += float(hits[1] - lastHits[1]) * m_settings.reward300;
	//	hitReward += float(hits[2] - lastHits[2]) * m_settings.reward200;
	//	hitReward += float(hits[3] - lastHits[3]) * m_settings.reward100;
	//	hitReward += float(hits[4] - lastHits[4]) * m_settings.reward50;

	//	if (hits[5] > lastHits[5]) 
	//	{
	//		const float diff = static_cast<float>(hits[5] - lastHits[5]);
	//		hitReward += (diff * m_settings.rewardMiss);
	//		m_hitStreak = 0.0f;
	//	}
	//	else 
	//	{
	//		m_hitStreak += m_settings.anyHitReward;
	//		hitReward += std::exp(m_hitStreak) * m_settings.hitStreakReward;
	//	}
	//}
	
	return movementReward + hitReward;
}

Neurome::Settings_t::Settings(std::string clientName, bool isAwaitProcess,
							  uint32_t awaitProcessDelay, std::string pauseHotKey, std::string modelPath,
							  uint32_t inputWidth, uint32_t inputHeight,
						 	  uint32_t hiddenSize, uint32_t epochs, uint32_t bufferSize,
					 	      float actorLr, float criticLr, float gamma, float epsilon,
						  	  float rewardPerfect, float reward300, float reward200,
							  float reward100, float reward50, float rewardMiss,
							  float lambda, float gradClip,
							  float movementNoise, float actionNoise,
							  float anyHitReward, float hitStreakReward, float targetMovement, float sigmaMovement)
	: clientName(clientName), isAwaitProcess(isAwaitProcess),
	awaitProcessDelay(awaitProcessDelay), pauseHotKey(pauseHotKey), modelPath(modelPath),
	inputWidth(inputWidth), inputHeight(inputHeight),
	hiddenSize(hiddenSize), epochs(epochs), bufferSize(bufferSize),
    actorLr(actorLr), criticLr(criticLr), gamma(gamma), epsilon(epsilon),
	rewardPerfect(rewardPerfect), reward300(reward300), reward200(reward200), 
	reward100(reward100), reward50(reward50), rewardMiss(rewardMiss),
	lambda(lambda), gradClip(gradClip), movementNoise(movementNoise), actionNoise(actionNoise),
    anyHitReward(anyHitReward), hitStreakReward(hitStreakReward), targetMovement(targetMovement), sigmaMovement(sigmaMovement) {}

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
	parseFloat(configHandler, &this->lambda, "lambda");
	parseFloat(configHandler, &this->gradClip, "gradClip");
	parseFloat(configHandler, &this->movementNoise, "movementNoise");
	parseFloat(configHandler, &this->actionNoise, "actionNoise");
	parseFloat(configHandler, &this->anyHitReward, "anyHitReward");
	parseFloat(configHandler, &this->hitStreakReward, "hitStreakReward");
	parseFloat(configHandler, &this->targetMovement, "targetMovement");
	parseFloat(configHandler, &this->sigmaMovement, "sigmaMovement");

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
	std::cout << " - bufferSize: " << bufferSize << std::endl;
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
	std::cout << " - lambda: " << lambda << std::endl;
	std::cout << " - gradClip: " << gradClip << std::endl;
	std::cout << " - movementNoise: " << movementNoise << std::endl;
	std::cout << " - actionNoise: " << actionNoise << std::endl;
	std::cout << " - anyHitReward: " << anyHitReward << std::endl;
	std::cout << " - hitStreakReward: " << hitStreakReward << std::endl;
	std::cout << " - targetMovement: " << targetMovement << std::endl;
	std::cout << " - sigmaMovement: " << sigmaMovement << std::endl;

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


=======
#include "Neurome.h"

Neurome::Neurome() noexcept
	: m_settingsPath("data/settings.cfg"),
	m_settings(), m_process()
{
	initSettings();
	m_settings.load(m_settingsPath);
}

Neurome::~Neurome() 
{
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

void Neurome::toWindowedMode() const
{
	ConfigHandler osuConfig(getCfgPath());
	osuConfig["Fullscreen"] = "0";

	osuConfig.save();
}

std::string Neurome::getProcessPath() const
{
	return m_process.getProcessPath();
}

int32_t Neurome::attachProcess()
{
	int32_t result = NONE;

	if (!m_process.getProcess(m_settings.clientName))
	{
		return result;
	}

	if (m_process.blockTraffic())
	{
		result |= BLOCK_TRAFFIC_MASK;
	}

	UserConfig_t userConfig;
	if (!userConfig.read(getCfgPath()))
	{
		m_process.release();

		return result | READ_USER_CONFIG_MASK;
	}

	if (userConfig.isFullscreen)
	{
		toWindowedMode();
		if (!m_process.restart())
		{
			result |= RESTART_MASK;
		}
	}

	return result | OK_MASK;
}

int32_t Neurome::detachProcess()
{
	m_process.release();
	
	return OK_MASK;
}

void Neurome::initSettings()
{
	m_settings.clientName = "osu!";
}

void Neurome::Settings_t::load(std::string path)
{
	if (!createFileDir(path))
	{
		return;
	}

	ConfigHandler configHandler(path);

	merge(&configHandler);
	configHandler.save();
}

void Neurome::Settings_t::save(std::string path) const
{
	ConfigHandler configHandler(path);
	configHandler.restore();

	configHandler["clientName"] = clientName;

	configHandler.save();
}

bool Neurome::Settings_t::merge(ConfigHandler *configHandler)
{
	if (!configHandler)
	{
		return false;
	}

	parseStr(configHandler, &clientName, "clientName");

	return true;
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

bool Neurome::UserConfig_t::read(std::string path)
{
	ConfigHandler osuConfig(path);

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

	this->keyLeft = keyLeft;
	this->isFullscreen = isFullscreenStr == "1";

	return true;
}
>>>>>>> 73860c5e4f984f52cafff7d349a394f3bb10aeba
