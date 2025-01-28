#include "ConfigHandler.h"

ConfigHandler::ConfigHandler(std::string path)
	: m_config(), m_path(path), m_isGood(false)
{
	std::ifstream file(path);
	if (!file.is_open())
	{
		return;
	}

	std::string line;
	while (std::getline(file, line))
	{
		line.erase(0, line.find_first_not_of(" \t"));

		if (line.empty() || line[0] == '#')
		{
			continue;
		}

		const size_t commentPos = line.find('#');
		if (commentPos != std::string::npos)
		{
			line = line.substr(0, commentPos);
		}

		const size_t equalsPos = line.find('=');
		if (equalsPos == std::string::npos)
		{
			file.close();
			return;
		}

		std::string key = line.substr(0, equalsPos);
		key.erase(key.find_last_not_of(" \t") + 1);

		std::string value = "";
		if (equalsPos + 1 < line.length())
		{
			value = line.substr(equalsPos + 1);

			value.erase(0, value.find_first_not_of(" \t"));
			value.erase(value.find_last_not_of(" \t") + 1);
		}

		m_config[key] = value;	
	}

	m_isGood = true;
	file.close();
}

ConfigHandler::~ConfigHandler() {}

bool ConfigHandler::restore()
{
	if (!recreateFile())
	{
		return false;
	}

	m_config.clear();

	return true;
}

bool ConfigHandler::save() const
{
	if (!recreateFile())
	{
		return false;
	}

	std::ofstream file(m_path);
	if (!file.is_open())
	{
		return false;
	}

	for (const auto& [key, value] : m_config)
	{
		file << key << " = " << value << '\n';
	}

	file.close();

	return true;
}

bool ConfigHandler::isGood() const
{
	return m_isGood;
}

const std::string& ConfigHandler::operator[](std::string key) const
{
	return m_config.at(key);
}

std::string& ConfigHandler::operator[](std::string key)
{
	const auto iter = m_config.find(key);
	if (iter == m_config.end()) 
	{
		m_config[key] = "";
	}

	return m_config[key];
}

bool ConfigHandler::recreateFile() const
{
	if (m_path.empty())
	{
		return false;
	}

	std::filesystem::path path(m_path);
	try
	{
		std::filesystem::create_directories(path.parent_path());
	}
	catch (...)
	{
		return false;
	}

	std::ofstream file(path, std::ios::trunc);
	if (!file.is_open())
	{
		return false;
	}

	file.close();

	return true;
}
