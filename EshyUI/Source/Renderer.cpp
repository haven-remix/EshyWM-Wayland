
#include <GL/glew.h>

#include "Renderer.h"
#include "IndexBuffer.h"
#include "VertexArray.h"
#include "Shader.h"
#include "Texture.h"
#include "Entity.h"

#include "glm/gtc/matrix_transform.hpp"

#include <iostream>

static glm::mat4 CreateMVPMatrix(const glm::mat4& ProjectionMatrix, const euiEntity* Entity)
{
	return ProjectionMatrix * glm::translate(glm::mat4(1.0f), glm::vec3(Entity->GetX(), Entity->GetY(), 0.0f));
}

euiRenderer::euiRenderer(GLFWwindow* _Window)
	: Window(_Window)
{
	int Width;
	int Height;
	glfwGetWindowSize(_Window, &Width, &Height);

	UpdateScreenSize((float)Width, (float)Height);
}

void euiRenderer::Clear() const
{
	glClear(GL_COLOR_BUFFER_BIT);
}

void euiRenderer::SetBackgroundColor(glm::vec4 Color)
{
	glClearColor(Color.r, Color.g, Color.b, Color.a);
}

void euiRenderer::Draw(const euiVertexArray& va, const euiIndexBuffer& ib, euiShader& shader)
{
	shader.Bind();
	shader.SetUniformMat4f("u_mvp", GetProjectionMatrix());
	va.Bind();
	ib.Bind();

	glDrawElements(GL_TRIANGLES, ib.GetCount(), GL_UNSIGNED_INT, nullptr);
}

void euiRenderer::Draw(class euiEntity* Entity, const glm::vec4 Color)
{
	Entity->GetShader()->Bind();
	Entity->GetShader()->SetUniformMat4f("u_mvp", CreateMVPMatrix(GetProjectionMatrix(), Entity));
	Entity->GetShader()->SetUniform4f("u_color", Color.r, Color.g, Color.b, Color.a);
	Entity->GetVertexArray()->Bind();
	Entity->GetIndexBuffer()->Bind();

	glDrawElements(GL_TRIANGLES, Entity->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, nullptr);
}

void euiRenderer::Draw(euiEntity* Entity, euiTexture* Texture)
{
	Entity->GetShader()->Bind();
	Entity->GetShader()->SetUniformMat4f("u_mvp", CreateMVPMatrix(GetProjectionMatrix(), Entity));
	Entity->GetVertexArray()->Bind();
	Entity->GetIndexBuffer()->Bind();

	if (Texture)
	{
		int Slot = Texture->Bind(this);
		Entity->GetShader()->SetUniform1i("u_texture", Slot);
	}

	glDrawElements(GL_TRIANGLES, Entity->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, nullptr);
}


void euiRenderer::UpdateScreenSize(float Width, float Height)
{
	WindowWidth = Width;
	WindowHeight = Height;
	ProjectionMatrix = glm::ortho(0.0f, Width, 0.0f, Height, -1.0f, 1.0f);
	glfwSetWindowSize(Window, (int)Width, (int)Height);
}

void euiRenderer::InformBindTexture(euiTexture* Texture, uint Slot)
{
	TextureBindingInfo.emplace(Slot, Texture);
}

int euiRenderer::GetFirstUnboundTextureSlot()
{
	int MaxTextureSlots;
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &MaxTextureSlots);

	if (TextureBindingInfo.size() > 0)
	{
		uint first = TextureBindingInfo.begin()->first;

		if (first > 0)
			return first - 1;
		else
		{
			uint last = TextureBindingInfo.rbegin()->first;

			if (last < (uint)MaxTextureSlots - 1)
				return last + 1;
		}
	}
	else
		return 0;

	return -1;
}
