
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Texture.h"
#include "Renderer.h"

#include "stb/stb_image.h"
#include <iostream>

euiTexture::euiTexture(const std::string& Path)
	: FilePath(Path)
	, LocalBuffer(nullptr)
	, BPP(0)
	, Bound(false)
	, Slot(0)
{
	InitializeTexture(Path);
}

euiTexture::~euiTexture()
{
	glDeleteTextures(1, &RendererID);
}

void euiTexture::Bind(uint slot /*= 0*/) const
{
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, RendererID);
}

int euiTexture::Bind(class euiRenderer* Renderer)
{
	if (!false)
	{
		Slot = Renderer->GetFirstUnboundTextureSlot();
		glActiveTexture(GL_TEXTURE0 + Slot);
		glBindTexture(GL_TEXTURE_2D, RendererID);
		Renderer->InformBindTexture(this, Slot);
		Bound = true;
	}

	return Slot;
}

void euiTexture::Unbind() const
{
	glActiveTexture(GL_TEXTURE0 + Slot);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void euiTexture::InitializeTexture(const std::string& Path)
{
	stbi_set_flip_vertically_on_load(1);
	LocalBuffer = stbi_load(Path.c_str(), &Width, &Height, &BPP, 4);

	glGenTextures(1, &RendererID);
	glBindTexture(GL_TEXTURE_2D, RendererID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, LocalBuffer);
	glBindTexture(GL_TEXTURE_2D, 0);

	if (LocalBuffer)
		stbi_image_free(LocalBuffer);
}
