
#include "VertexArray.h"
#include "EshyUI.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Shader.h"
#include "Texture.h"
#include "Renderer.h"
#include "Entity.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <GLFW/glfw3.h>

#include <string>

int main()
{
	GLFWwindow* window = InitEshyUI("Eshybar", 1280.0f, 720.0f);

	euiRenderer renderer(window);

	euiSolidEntity sentity(20.0f, 100.0f, 200.0f, 200.0f, glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
	euiImageEntity entity(20.0f, 100.0f, 200.0f, 200.0f, &renderer, "Resources\\image1.png");
	euiSolidEntity sentity2(240.0f, 100.0f, 200.0f, 200.0f, glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
	euiImageEntity entity2(240.0f, 100.0f, 200.0f, 200.0f, &renderer, "Resources\\image2.png");
	euiSolidEntity sentity3(460.0f, 100.0f, 200.0f, 200.0f, glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
	euiImageEntity entity3(460.0f, 100.0f, 200.0f, 200.0f, &renderer, "Resources\\image3.png");
	euiSolidEntity sentity4(680.0f, 100.0f, 200.0f, 200.0f, glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
	euiImageEntity entity4(680.0f, 100.0f, 200.0f, 200.0f, &renderer, "Resources\\image4.png");
	euiSolidEntity sentity5(900.0f, 100.0f, 200.0f, 200.0f, glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
	euiImageEntity entity5(900.0f, 100.0f, 200.0f, 200.0f, &renderer, "Resources\\image5.png");

	while (!glfwWindowShouldClose(window))
	{
		renderer.Clear();

		sentity.Draw(renderer);
		entity.Draw(renderer);
		sentity2.Draw(renderer);
		entity2.Draw(renderer);
		sentity3.Draw(renderer);
		entity3.Draw(renderer);
		sentity4.Draw(renderer);
		entity4.Draw(renderer);
		sentity5.Draw(renderer);
		entity5.Draw(renderer);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	Shutdown();
	return 0;
}