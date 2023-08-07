
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "EshyUI.h"
#include "Util.h"

GLFWwindow* InitEshyUI(const std::string& WindowName, float Width, float Height)
{
	if (!glfwInit())
		return nullptr;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow((int)Width, (int)Height, WindowName.c_str(), NULL, NULL);
	eshyui_glfw_check(window);
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	eshyui_glfw_check(glewInit() == GLEW_OK);

#ifdef ESHYUI_PRINT_OPENGL_VERSION
	std::cout << "Version:" << glGetString(GL_VERSION) << std::endl;
#endif

	return window;
}

void Shutdown()
{
	glfwTerminate();
}