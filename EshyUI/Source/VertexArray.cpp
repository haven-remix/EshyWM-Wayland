
#include "VertexArray.h"
#include "VertexBuffer.h"

euiVertexArray::euiVertexArray()
{
	glGenVertexArrays(1, &RendererID);
}

euiVertexArray::~euiVertexArray()
{
	glDeleteVertexArrays(1, &RendererID);
}

void euiVertexArray::Bind() const
{
	glBindVertexArray(RendererID);
}

void euiVertexArray::Unbind() const
{
	glBindVertexArray(0);
}

void euiVertexArray::AddBuffer(const euiVertexBuffer& vb, const euiVertexBufferLayout& vbl)
{
	Bind();
	vb.Bind();

	uint64_t offset = 0;
	for (uint i = 0; i < vbl.GetElement().size(); ++i)
	{
		glEnableVertexAttribArray(i);
		glVertexAttribPointer(i, vbl.GetElement()[i].Count, vbl.GetElement()[i].Type, vbl.GetElement()[i].Normalized, vbl.GetStride(), (const void*)offset);
		offset += vbl.GetElement()[i].Count * sizeof(vbl.GetElement()[i].Type);
	}
}
