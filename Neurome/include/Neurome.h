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
