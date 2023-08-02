
#include "eshybar.h"
#include "eshyui.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <GLES3/gl3.h>

#include <linux/input.h>

#include <iostream>

static std::string vertex_shader = R"gls(
	#version 330 core

	layout(location = 0) in vec4 position;
	layout(location = 1) in vec2 tex_coord;

	out vec2 v_tex_coord;

	uniform mat4 u_mvp;

	void main()
	{
		gl_Position = u_mvp * position;
		v_tex_coord = tex_coord;
	}
)gls";

static std::string fragment_shader = R"gls(
	#version 330 core

	layout(location = 0) out vec4 color;

	in vec2 v_tex_coord;

	uniform vec4 u_color;
	uniform sampler2D u_texture;

	void main()
	{
		vec4 tex_color = texture(u_texture, v_tex_coord);
		color = tex_color;
	}
)gls";

static Eshybar* eshybar;

Eshybar::Eshybar()
{
    
}

Eshybar::~Eshybar()
{
	
}

int main()
{
	GLFWwindow* window = eshyui::init_eshyui();

	float positions[] = {
		-0.5f, -0.5f, 0.0f, 0.0f,
		 0.5f, -0.5f, 1.0f, 0.0f,
		 0.5f,  0.5f, 1.0f, 1.0f,
		-0.5f,  0.5f, 0.0f, 1.0f
	};

	uint indicies[] = {
		0, 1, 2,
		2, 3, 0
	};

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	eshyui::vertex_array va;
	eshyui::vertex_buffer vb(positions, 4 * 4 * sizeof(float));
	eshyui::vertex_buffer_layout layout;
	layout.push<float>(2);
	layout.push<float>(2);
	va.add_buffer(vb, layout);
	eshyui::index_buffer ib(indicies, 6);

	glm::mat4 proj = glm::ortho(-2.0f, 2.0f, -1.5f, 1.5f, -1.0f, 1.0f);

	eshyui::shader shader(vertex_shader, fragment_shader);
	shader.bind();
	shader.set_uniform_4f("u_color", 0.2f, 0.3f, 0.8f, 1.0f);
	shader.set_uniform_mat4f("u_mvp", proj);

	eshyui::texture texture("image.png");
	texture.bind();
	shader.set_uniform_1i("u_texture", 0);

	va.unbind();
	shader.unbind();
	vb.unbind();
	ib.unbind();

	eshyui::renderer renderer;

	float r = 0.0f;
	float increment = 0.05f;
    while (!glfwWindowShouldClose(window))
    {
		renderer.clear();
		
		shader.bind();
		shader.set_uniform_4f("u_color", r, 0.3f, 0.8f, 1.0f);

		renderer.draw(va, ib, shader);

		if (r > 1.0f)
			increment = -0.05f;
		if (r < 0.0f)
			increment = 0.05f;

		r += increment;

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

	eshyui::shutdown();
	return 0;
}