
#pragma once

typedef unsigned int uint;

#define eshyui_glfw_check(condition)       if (!(condition)) {glfwTerminate(); return nullptr;}