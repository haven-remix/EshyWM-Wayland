
#pragma once

#include "Util.h"

#include <GLFW/glfw3.h>

#include <string>

extern GLFWwindow* InitEshyUI(const std::string& WindowName, float Width, float Height);
extern void Shutdown();
