
#pragma once

#include "Util.h"

class euiVertexBuffer
{
public:

	euiVertexBuffer() {}
	euiVertexBuffer(const void* Data, uint Size);
	~euiVertexBuffer();

	void Bind() const;
	void Unbind() const;

	void InitailizeVertexBuffer(const void* Data, uint Size);

private:

	uint RendererID;
};

