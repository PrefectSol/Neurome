#include "Neurome.h"

Neurome::Neurome() noexcept
	: m_settings()
{
	m_settings.load("data/settings.cfg");
}

Neurome::~Neurome() 
{
}

Neurome::Settings_t::Settings() {}

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

bool Neurome::Settings_t::merge(ConfigHandler *configHandler)
{
	if (!configHandler)
	{
		return false;
	}

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