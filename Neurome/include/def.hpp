#ifndef __DEF_HPP
#define __DEF_HPP 

#include <cstdint>

#include "imgui.h"

namespace env
{
	constexpr const char *windowTitle = "Better than a command line! (or not?)";
	constexpr const char *glslVersion = "#version 130";
	constexpr const char *fontFile = "data/fonts/Roboto-Medium.ttf";

	constexpr const char *minButtonTitle = "min";
	constexpr const char *maxButtonTitle = "max";
	constexpr const char *closeButtonTitle = "close";

	constexpr const uint32_t minWindowWidth = 128;
	constexpr const uint32_t minWindowHeight = 128;

	constexpr const uint32_t visualBorderSize = 2;
	constexpr const uint32_t resizeHitbox = 2;

	constexpr const int32_t vsync = 1;
	constexpr const int32_t glfwSleepMs = 10;

	constexpr const int32_t oversampleH = 1;
	constexpr const int32_t oversampleV = 1;

	constexpr const float initialWindowOffset = 0.1f;
	constexpr const float pixelsSize = 15.0f;

	constexpr const float borderColorR = 0.11f;
	constexpr const float borderColorG = 0.11f;
	constexpr const float borderColorB = 0.11f;
	constexpr const float borderColorA = 1.00f;

	constexpr const float clearColorR = 0.12f;
	constexpr const float clearColorG = 0.15f;
	constexpr const float clearColorB = 0.18f;
	constexpr const float clearColorA = 1.00f;

	constexpr const float buttonStyleR = 0.2f;
	constexpr const float buttonStyleG = 0.2f;
	constexpr const float buttonStyleB = 0.2f;
	constexpr const float buttonStyleA = 0.6f;

	constexpr const float buttonHoveredStyleR = 0.3f;
	constexpr const float buttonHoveredStyleG = 0.3f;
	constexpr const float buttonHoveredStyleB = 0.3f;
	constexpr const float buttonHoveredStyleA = 0.8f;

	constexpr const float buttonActiveStyleR = 0.4f;
	constexpr const float buttonActiveStyleG = 0.4f;
	constexpr const float buttonActiveStyleB = 0.4f;
	constexpr const float buttonActiveStyleA = 1.0f;

	constexpr const float buttonSpacing = 30.0f;

	constexpr const bool pixelSnapH = true;
};

#endif // !__DEF_HPP
