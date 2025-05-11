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

#include "def.hpp"
#include "Neurome.h"
#include "Messenger.h"

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glfw3.lib")

#define MAX_STR 256

class NeuromeGUI //: private Neurome
{
public:
	explicit NeuromeGUI() noexcept;

	~NeuromeGUI();

	void render();

private:
	static void glfwErrorCallback(int32_t error, const char *description);

	static void glfwFramebufferSizeCallback(GLFWwindow *window, int32_t width, int32_t height);

	enum GlyphRange : ImWchar 
	{
		None = 0,

		BasicLatinStart = 0x0020,
		BasicLatinEnd = 0x007F,

		Latin1SupplementStart = 0x0080,
		Latin1SupplementEnd = 0x00FF,

		CyrillicStart = 0x0400,
		CyrillicEnd = 0x04FF,

		CyrillicSupplementStart = 0x0500,
		CyrillicSupplementEnd = 0x052F
	};

	enum class Direction
	{
		None = 0,
		Left,
		Right,
		Top,
		Bottom,
		TopLeft,
		TopRight,
		BottomLeft,
		BottomRight
	};

	typedef struct WindowDeleter
	{
		void operator()(GLFWwindow *window);
	} WindowDeleter_t;

	typedef struct CursorDeleter
	{
		void operator()(GLFWcursor *cursor);
	} CursorDeleter_t;

	int32_t m_windowWidth;
	int32_t m_windowHeight;

	std::unique_ptr<GLFWwindow, WindowDeleter_t> m_window;

	std::unique_ptr<GLFWcursor, CursorDeleter_t> m_ewResizeCursor;
	std::unique_ptr<GLFWcursor, CursorDeleter_t> m_nsResizeCursor;
	std::unique_ptr<GLFWcursor, CursorDeleter_t> m_nwseResizeCursor;
	std::unique_ptr<GLFWcursor, CursorDeleter_t> m_neswResizeCursor;

	bool m_isInitialized;

	bool m_isResizing;
	Direction m_resizeDirection;
	int32_t m_windowStartX, m_windowStartY;
	int32_t m_windowStartWidth, m_windowStartHeight;

	bool m_isDragging;
	int32_t m_windowX, m_windowY;
	double m_dragStartX, m_dragStartY;

	//float m_settingsWidth;
	//float m_canvasHeight;
	//float m_manageWidth;

	//bool m_isChangedSettings;
	//bool m_isAttachProcess;
	//bool m_isCaptureWindow;

	void update();
	
	void setInitialWindowPos() const;

	void setColorStyle() const;

	void drawBorder() const;

	void handleResize();

	void updateResizeDirection();

	void updateCursor();

	bool getWindowConstraints(float x, float y, int32_t *newX, int32_t *newY,
							  int32_t *newWidth, int32_t *newHeight) const;

	void setWindowConstraints(int32_t newX, int32_t newY, int32_t newWidth, int32_t newHeight);

	void handleDragging();

	float beginMenuBar();

	//void beginSettings(float menuBarHeight, ImGuiWindowFlags windowFlags);

	//void beginCanvas(float menuBarHeight, ImGuiWindowFlags windowFlags);

	//void beginManage(float menuBarHeight, ImGuiWindowFlags windowFlags);

	//void beginInfo(float menuBarHeight, ImGuiWindowFlags windowFlags);

	//void toggleProcess();
};
#endif // !__NEUROME_GUI_H
