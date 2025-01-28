#ifndef __CONFIG_HANDLER_H
#define __CONFIG_HANDLER_H

#include <string>
#include <map>
#include <fstream>
#include <filesystem>

class ConfigHandler
{
public:
	explicit ConfigHandler(std::string path);

	~ConfigHandler();

	bool restore();

	bool save() const;

	bool isGood() const;

	const std::string& operator[](std::string key) const;

	std::string& operator[](std::string key);

private:
	std::map<std::string, std::string> m_config;

	std::string m_path;

	bool m_isGood;

	bool recreateFile() const;
};

#endif // !__CONFIG_HANDLER_H