
#pragma once

#include "Util.h"

class euiIndexBuffer
{
public:

	euiIndexBuffer() {}
	euiIndexBuffer(const uint* Data, uint Count);
	~euiIndexBuffer();

	void Bind() const;
	void Unbind() const;

	void InitializeIndexBuffer(const uint* Data, uint Count);

	inline uint GetCount() const {return Count;}

private:

	uint RendererID;
	uint Count;
};

