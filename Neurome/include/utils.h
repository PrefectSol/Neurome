#ifndef __UTILS_H
#define __UTILS_H

#include <string>
#include <algorithm>
#include <unordered_map>
#include <sstream>

#include <Windows.h>

std::string toLower(const std::string &str);

std::string boolToString(bool value);

bool isPositiveInteger(const std::string &str);

bool isFloat(const std::string &str);

bool verifyKey(const std::string &str);

std::string makeLine(uint32_t width, char symbol);

#endif // !__UTILS_H
