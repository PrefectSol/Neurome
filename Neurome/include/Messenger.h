#ifndef __MESSENGER_H
#define __MESSENGER_H

#include <GLFW/glfw3.h>
#include <Windows.h>

#include "imgui.h"

class Messenger
{
public:
	static void error(const char *content);

	static void warning(const char *content);

	static void info(const char *content);

private:
	static void modalBox(const char *title, const char *content, const ImVec4 &color);
};
#endif // !__MESSENGER_H
