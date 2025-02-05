#include "utils.h"

std::string toLower(const std::string &str)
{
	std::string lowerStr = str;
	std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), [](unsigned char c) { return std::tolower(c); });

	return lowerStr;
}

std::string boolToString(bool value)
{
	return value ? "true" : "false";
}

bool isPositiveInteger(const std::string &str)
{
	std::size_t pos, num;
	try
	{
		num = std::stoi(str, &pos);
	}
	catch (...)
	{
		return false;
	}

	return pos == str.length() && num > 0;
}

bool isFloat(const std::string &str)
{
	std::size_t pos;
	try
	{
		std::stof(str, &pos);
	}
	catch (...)
	{
		return false;
	}

	return pos == str.length();
}

bool verifyKey(const std::string &str) 
{

	const std::unordered_map<std::string, int> validKeys = 
	{
		{"F1", VK_F1}, {"F2", VK_F2}, {"F3", VK_F3},
		{"F4", VK_F4}, {"F5", VK_F5}, {"F6", VK_F6},
		{"F7", VK_F7}, {"F8", VK_F8}, {"F9", VK_F9},
		{"F10", VK_F10}, {"F11", VK_F11}, {"F12", VK_F12},

		{"ALT", VK_MENU}, {"CTRL", VK_CONTROL},
		{"SHIFT", VK_SHIFT}, {"WIN", VK_LWIN},

		{"A", 'A'}, {"B", 'B'}, {"C", 'C'}, {"D", 'D'},
		{"E", 'E'}, {"F", 'F'}, {"G", 'G'}, {"H", 'H'},
		{"I", 'I'}, {"J", 'J'}, {"K", 'K'}, {"L", 'L'},
		{"M", 'M'}, {"N", 'N'}, {"O", 'O'}, {"P", 'P'},
		{"Q", 'Q'}, {"R", 'R'}, {"S", 'S'}, {"T", 'T'},
		{"U", 'U'}, {"V", 'V'}, {"W", 'W'}, {"X", 'X'},
		{"Y", 'Y'}, {"Z", 'Z'}
	};

	if (validKeys.count(str))
	{
		return true;
	}

	if (str.find('+') != std::string::npos) 
	{
		std::istringstream iss(str);

		std::string key;
		while (std::getline(iss, key, '+')) 
		{
			if (!validKeys.count(key)) 
			{
				return false;	
			}
		}

		return true;
	}
	return false;
}

std::string makeLine(uint32_t width, char symbol)
{
	return std::string(width, symbol);
}