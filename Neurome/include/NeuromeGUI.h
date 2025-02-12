#ifndef __NEUROME_GUI_H
#define __NEUROME_GUI_H

#include <cstdint>
#include <string>
#include <exception>
#include <locale>

#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "Neurome.h"
#include "Messenger.h"

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glfw3.lib")

#define MAX_STR 256

class NeuromeGUI : private Neurome
{
public:
	explicit NeuromeGUI() noexcept;

	~NeuromeGUI();

	void render();

private:
	static void glfwErrorCallback(int error, const char *description);

	const std::string m_windowTitle;

	int m_windowWidth;
	int m_windowHeight;

	bool m_initalized;

	GLFWwindow *m_window;

	float m_settingsWidth;
	float m_canvasHeight;
	float m_manageWidth;

	bool m_isDragging;
	double m_dragStartX;
	double m_dragStartY;

	void update();
	
	void setInitialWindowPos() const;

	float beginMenuBar();

	void checkDragging();

	void beginSettings(float menuBarHeight, ImGuiWindowFlags windowFlags);

	void beginCanvas(float menuBarHeight, ImGuiWindowFlags windowFlags);

	void beginManage(float menuBarHeight, ImGuiWindowFlags windowFlags);

	void beginInfo(float menuBarHeight, ImGuiWindowFlags windowFlags);
};
#endif // !__NEUROME_GUI_H
