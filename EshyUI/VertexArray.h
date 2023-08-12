
#pragma once

#include "Util.h"

#include <GL/glew.h>

#include <vector>

struct euiVertexBufferElement
{
	uint Type;
	uint Count;
	unsigned char Normalized;
};

class euiVertexBufferLayout
{
public:

	euiVertexBufferLayout()
		: Stride(0)
	{}

	template<typename T>
	void push(uint count)
	{
		//static_assert(false);
	}

	inline const std::vector<euiVertexBufferElement>& GetElement() const {return Elements;}
	inline uint GetStride() const {return Stride;}

private:

	std::vector<euiVertexBufferElement> Elements;
	uint Stride;
};

template<>
inline void euiVertexBufferLayout::push<float>(uint count)
{
	Elements.push_back({GL_FLOAT, count, false});
	Stride += count * sizeof(float);
}

template<>
inline void euiVertexBufferLayout::push<uint>(uint count)
{
	Elements.push_back({GL_UNSIGNED_INT, count, false});
	Stride += count * sizeof(uint);
}

template<>
inline void euiVertexBufferLayout::push<unsigned char>(uint count)
{
	Elements.push_back({GL_UNSIGNED_BYTE, count, true});
	Stride += count * sizeof(unsigned char);
}

class euiVertexArray
{
public:

	euiVertexArray();
	~euiVertexArray();

	void Bind() const;
	void Unbind() const;

	void AddBuffer(const class euiVertexBuffer& vb, const euiVertexBufferLayout& vbl);

private:

	uint RendererID;
};

