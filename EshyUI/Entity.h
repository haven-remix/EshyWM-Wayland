
#pragma once

#include "glm/glm.hpp"

#include <string>
#include <functional>

class euiEntity;

typedef void (*euiEntityEventCallback)(euiEntity*, void*);

enum euiAnchor
{
	EUI_NONE,
	EUI_ANCHOR_CENTER,
	EUI_ANCHOR_LEFT_TOP,
	EUI_ANCHOR_LEFT_MIDDLE,
	EUI_ANCHOR_LEFT_BOTTOM,
	EUI_ANCHOR_RIGHT_TOP,
	EUI_ANCHOR_RIGHT_MIDDLE,
	EUI_ANCHOR_RIGHT_BOTTOM,
	EUI_ANCHOR_TOP_MIDDLE,
	EUI_ANCHOR_BOTTOM_MIDDLE
};

class euiEntity
{
public:

	euiEntity(float x, float y, float Width, float Height, euiAnchor Anchor);

	virtual void Draw(class euiRenderer& Renderer);

	void SetPosition(float x, float y);
	void SetSize(float Width, float Height);

	virtual void NotifyPointerEnter();
	virtual void NotifyPointerExit();
	virtual void NotifyPointerHover();
	virtual void NotifyPointerPressed();
	virtual void NotifyPointerUnpressed();
	virtual void NotifyPointerHeld();
	void NotifyPointerStatus(bool bCurrentlyHovering, bool bCurrentlyPressed);
	void CheckPointerStatus(double PointerXPos, double PointerYPos, bool bLeftButtonPressed);

	class euiVertexArray* GetVertexArray() {return VertexArray;}
	class euiVertexBuffer* GetVertexBuffer() {return VertexBuffer;}
	class euiVertexBufferLayout* GetLayout() {return Layout;}
	class euiIndexBuffer* GetIndexBuffer() {return IndexBuffer;}
	class euiShader* GetShader() {return Shader;}

	float GetX() const {return x;}
	float GetY() const {return y;}
	float GetWidth() const {return Width;}
	float GetHeight() const {return Height;}

	euiEntityEventCallback NotifyPointerEnterCallback;
	euiEntityEventCallback NotifyPointerExitCallback;
	euiEntityEventCallback NotifyPointerHoverCallback;
	euiEntityEventCallback NotifyPointerPressedCallback;
	euiEntityEventCallback NotifyPointerUnpressedCallback;
	euiEntityEventCallback NotifyPointerHeldCallback;
	void* CallbackData;

protected:

	float x;
	float y;
	float Width;
	float Height;
	euiAnchor Anchor;

	bool bPointerIsHovering;
	bool bPointerIsPressed;

	class euiVertexArray* VertexArray;
	class euiVertexBuffer* VertexBuffer;
	class euiVertexBufferLayout* Layout;
	class euiIndexBuffer* IndexBuffer;
	class euiShader* Shader;

	void GetAnchorAdjustedPosition(float x, float y, float& NewX, float& NewY) const;
};

class euiSolidEntity : public euiEntity
{
public:

	euiSolidEntity(float x, float y, float Width, float Height, euiAnchor Anchor);

	virtual void Draw(class euiRenderer& Renderer) override;

	void ForceUpdateColor();
	void SetNormalColor(const glm::vec4& Color);
	void SetNormalColor(float r, float g, float b, float a);
	void SetHoveredColor(const glm::vec4& Color);
	void SetHoveredColor(float r, float g, float b, float a);
	void SetPressedColor(const glm::vec4& Color);
	void SetPressedColor(float r, float g, float b, float a);

	virtual void NotifyPointerEnter() override;
	virtual void NotifyPointerExit() override;
	virtual void NotifyPointerPressed() override;
	virtual void NotifyPointerUnpressed() override;

private:

	glm::vec4 CurrentColor;
	glm::vec4 NormalColor;
	glm::vec4 HoveredColor;
	glm::vec4 PressedColor;
};

class euiImageEntity : public euiEntity
{
public:

	euiImageEntity(float x, float y, float Width, float Height, euiAnchor Anchor, euiRenderer* Renderer, const std::string& ImagePath);

	virtual void Draw(class euiRenderer& Renderer) override;

private:

	class euiTexture* Texture;
};