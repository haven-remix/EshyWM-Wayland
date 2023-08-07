
#include "IndexBuffer.h"

#include <GL/glew.h>

euiIndexBuffer::euiIndexBuffer(const uint* Data, uint _Count)
	: Count(_Count)
{
	InitializeIndexBuffer(Data, Count);
}

euiIndexBuffer::~euiIndexBuffer()
{
	glDeleteBuffers(1, &RendererID);
}

void euiIndexBuffer::Bind() const
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, RendererID);
}

void euiIndexBuffer::Unbind() const
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void euiIndexBuffer::InitializeIndexBuffer(const uint* Data, uint Count)
{
	glGenBuffers(1, &RendererID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, RendererID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, Count * sizeof(uint), Data, GL_STATIC_DRAW);
}
