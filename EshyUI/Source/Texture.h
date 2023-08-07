
#pragma once

#include "Util.h"

#include <string>

class euiTexture
{
public:

	euiTexture() {}
	euiTexture(const std::string& Path);
	~euiTexture();

	void Bind(uint slot = 0) const;
	int Bind(class euiRenderer* Renderer);
	void Unbind() const;

	void InitializeTexture(const std::string& Path);

	inline int GetWidth() const {return Width;}
	inline int GetHeight() const {return Height;}
	inline int GetSlot() const {return Slot;}
	inline uint GetRendererID() const {return RendererID;}

private:

	uint RendererID;
	std::string FilePath;
	unsigned char* LocalBuffer;
	int Width;
	int Height;
	int BPP;
	bool Bound;
	int Slot;
};

