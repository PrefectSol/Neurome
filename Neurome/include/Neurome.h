#ifndef __NEUROME_H
#define __NEUROME_H

#include "ConfigHandler.h"
#include "ProcessHandler.h"
#include "utils.h"

class Neurome
{
public:
	explicit Neurome() noexcept;

	~Neurome();

private:
	typedef struct Settings
	{
	public:
		explicit Settings();

		void load(std::string path);

	private:
		bool merge(ConfigHandler *configHandler);

		void parseStr(ConfigHandler *configHandler, std::string *value, std::string field);

		void parseUInt(ConfigHandler *configHandler, uint32_t *value, std::string field);

		void parseFloat(ConfigHandler *configHandler, float *value, std::string field);

		void parseBool(ConfigHandler *configHandler, bool *value, std::string field);

		void parseKey(ConfigHandler *configHandler, std::string *value, std::string field);
	} Settings_t;
	
protected:
	Settings_t m_settings;
};
#endif // !__NEUROME_H
