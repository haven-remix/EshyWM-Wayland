
#include "VertexBuffer.h"

#include <GL/glew.h>

euiVertexBuffer::euiVertexBuffer(const void* Data, uint Size)
{
	InitailizeVertexBuffer(Data, Size);
}

euiVertexBuffer::~euiVertexBuffer()
{
	glDeleteBuffers(1, &RendererID);
}

void euiVertexBuffer::Bind() const
{
	glBindBuffer(GL_ARRAY_BUFFER, RendererID);
}

void euiVertexBuffer::Unbind() const
{
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void euiVertexBuffer::InitailizeVertexBuffer(const void* Data, uint Size)
{
	glGenBuffers(1, &RendererID);
	glBindBuffer(GL_ARRAY_BUFFER, RendererID);
	glBufferData(GL_ARRAY_BUFFER, Size, Data, GL_STATIC_DRAW);
}
