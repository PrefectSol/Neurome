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