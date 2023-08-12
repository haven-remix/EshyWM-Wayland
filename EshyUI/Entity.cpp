
#include "Entity.h"
#include "VertexArray.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Shader.h"
#include "Texture.h"
#include "Renderer.h"

#include "glm/gtc/matrix_transform.hpp"

#include <iostream>

euiEntity::euiEntity(float _x, float _y, float _Width, float _Height, euiAnchor _Anchor)
	: x(_x)
	, y(_y)
	, Width(_Width)
	, Height(_Height)
	, Anchor(_Anchor)
	, bPointerIsHovering(false)
	, bPointerIsPressed(false)
	, VertexArray(nullptr)
	, VertexBuffer(nullptr)
	, IndexBuffer(nullptr)
	, Layout(nullptr)
	, Shader(nullptr)
{}

void euiEntity::Draw(euiRenderer& Renderer)
{
	Renderer.Draw(this);
}

void euiEntity::SetPosition(float NewX, float NewY)
{
	float AdjustedX;
	float AdjustedY;
	GetAnchorAdjustedPosition(NewX, NewY, AdjustedX, AdjustedY);

	x = AdjustedX;
	y = AdjustedY;
}

void euiEntity::SetSize(float NewWidth, float NewHeight)
{
	Width = NewWidth;
	Height = NewHeight;
}

void euiEntity::NotifyPointerEnter()
{
	if (NotifyPointerEnterCallback)
		NotifyPointerEnterCallback(this, CallbackData);
}

void euiEntity::NotifyPointerExit()
{
	if (NotifyPointerExitCallback)
		NotifyPointerExitCallback(this, CallbackData);
}

void euiEntity::NotifyPointerHover()
{
	if (NotifyPointerHoverCallback)
		NotifyPointerHoverCallback(this, CallbackData);
}

void euiEntity::NotifyPointerPressed()
{
	if (NotifyPointerPressedCallback)
		NotifyPointerPressedCallback(this, CallbackData);
}

void euiEntity::NotifyPointerUnpressed()
{
	if (NotifyPointerUnpressedCallback)
		NotifyPointerUnpressedCallback(this, CallbackData);
}

void euiEntity::NotifyPointerHeld()
{
	if (NotifyPointerHeldCallback)
		NotifyPointerHeldCallback(this, CallbackData);
}

void euiEntity::NotifyPointerStatus(bool bCurrentlyHovering, bool bCurrentlyPressed)
{
	if (!bPointerIsHovering && bCurrentlyHovering)
	{
		bPointerIsHovering = true;
		NotifyPointerEnter();
	}
	else if (bPointerIsHovering && !bCurrentlyHovering)
	{
		bPointerIsHovering = false;
		NotifyPointerExit();
	}
	else if (bPointerIsHovering && bCurrentlyHovering)
	{
		NotifyPointerHover();
	}

	if (!bPointerIsPressed && bCurrentlyPressed)
	{
		bPointerIsPressed = true;
		NotifyPointerPressed();
	}
	else if (bPointerIsPressed && !bCurrentlyPressed)
	{
		bPointerIsPressed = false;
		NotifyPointerUnpressed();
	}
	else if (bPointerIsPressed && bCurrentlyPressed)
	{
		NotifyPointerHeld();
	}
}

void euiEntity::CheckPointerStatus(double PointerXPos, double PointerYPos, bool bLeftButtonPressed)
{
	const bool bIsHovered = PointerXPos > GetX() && PointerXPos < GetX() + GetWidth() && PointerYPos > GetY() && PointerYPos < GetY() + GetHeight();

	if (!bPointerIsHovering && bIsHovered)
	{
		bPointerIsHovering = true;
		NotifyPointerEnter();
	}
	else if (bPointerIsHovering && !bIsHovered)
	{
		bPointerIsHovering = false;
		NotifyPointerExit();
	}
	else if (bPointerIsHovering && bIsHovered)
	{
		NotifyPointerHover();
	}

	if (!bPointerIsPressed && bLeftButtonPressed && bIsHovered)
	{
		bPointerIsPressed = true;
		NotifyPointerPressed();
	}
	else if (bPointerIsPressed && !(bLeftButtonPressed && bIsHovered))
	{
		bPointerIsPressed = false;
		NotifyPointerUnpressed();
	}
	else if (bPointerIsPressed && bLeftButtonPressed && bIsHovered)
	{
		NotifyPointerHeld();
	}
}

void euiEntity::GetAnchorAdjustedPosition(float _x, float _y, float& NewX, float& NewY) const
{
	float AdjustedX = 0.0f;
	float AdjustedY = 0.0f;

	switch (Anchor)
	{
	case EUI_ANCHOR_CENTER:
	{
		AdjustedX = _x + -(Width / 2);
		AdjustedY = _y + -(Height / 2);
		break;
	}
	case EUI_ANCHOR_LEFT_TOP:
	{
		AdjustedX = _x;
		AdjustedY = _y - Height;
		break;
	}
	case EUI_ANCHOR_LEFT_MIDDLE:
	{
		AdjustedX = _x;
		AdjustedY = _y - (Height / 2);
		break;
	}
	case EUI_ANCHOR_LEFT_BOTTOM:
	{
		AdjustedX = _x;
		AdjustedY = _y;
		break;
	}
	case EUI_ANCHOR_RIGHT_TOP:
	{
		AdjustedX = _x - Width;
		AdjustedY = _y - Height;
		break;
	}
	case EUI_ANCHOR_RIGHT_MIDDLE:
	{
		AdjustedX = _x - Width;
		AdjustedY = _y - (Height / 2);
		break;
	}
	case EUI_ANCHOR_RIGHT_BOTTOM:
	{
		AdjustedX = _x - Width;
		AdjustedY = _y;
		break;
	}
	case EUI_ANCHOR_TOP_MIDDLE:
	{
		AdjustedX = _x + -(Width / 2);
		AdjustedY = _y - Height;
		break;
	}
	case EUI_ANCHOR_BOTTOM_MIDDLE:
	{
		AdjustedX = _x + -(Width / 2);
		AdjustedY = _y;
		break;
	}
	};

	NewX = AdjustedX;
	NewY = AdjustedY;
}


euiSolidEntity::euiSolidEntity(float _x, float _y, float _Width, float _Height, euiAnchor _Anchor)
	: euiEntity(_x, _y, _Width, _Height, _Anchor)
{
	SetPosition(_x, _y);

	float positions[] = {
		 0.0f,		    0.0f,			//Bottom left
		 0.0f + _Width, 0.0f,			//Bottom right
		 0.0f + _Width, 0.0f + _Height, //Top right
		 0.0f,		    0.0f + _Height, //Top left
	};

	uint indicies[] = {
		0, 1, 2,
		2, 3, 0
	};

	VertexBuffer = new euiVertexBuffer(positions, 4 * 2 * sizeof(float));

	Layout = new euiVertexBufferLayout();
	Layout->push<float>(2);

	VertexArray = new euiVertexArray();
	VertexArray->AddBuffer(*VertexBuffer, *Layout);

	IndexBuffer = new euiIndexBuffer(indicies, 6);
	IndexBuffer->InitializeIndexBuffer(indicies, 6);

	Shader = new euiShader("./Resources/solid.vshader", "./Resources/solid.fshader");

	VertexArray->Unbind();
	Shader->Unbind();
	VertexBuffer->Unbind();
	IndexBuffer->Unbind();
}

void euiSolidEntity::Draw(class euiRenderer& Renderer)
{
	Renderer.Draw(this, CurrentColor);
}

void euiSolidEntity::ForceUpdateColor()
{
	if (bPointerIsHovering && !bPointerIsPressed)
		CurrentColor = HoveredColor;
	else if (bPointerIsHovering && bPointerIsPressed)
		CurrentColor = PressedColor;
	else
		CurrentColor = NormalColor;
}

void euiSolidEntity::SetNormalColor(const glm::vec4& Color)
{
	NormalColor = Color;
	ForceUpdateColor();
}

void euiSolidEntity::SetNormalColor(float r, float g, float b, float a)
{
	NormalColor = glm::vec4(r, g, b, a);
	ForceUpdateColor();
}

void euiSolidEntity::SetHoveredColor(const glm::vec4& Color)
{
	HoveredColor = Color;
	ForceUpdateColor();
}

void euiSolidEntity::SetHoveredColor(float r, float g, float b, float a)
{
	HoveredColor = glm::vec4(r, g, b, a);
	ForceUpdateColor();
}

void euiSolidEntity::SetPressedColor(const glm::vec4& Color)
{
	PressedColor = Color;
	ForceUpdateColor();
}

void euiSolidEntity::SetPressedColor(float r, float g, float b, float a)
{
	PressedColor = glm::vec4(r, g, b, a);
	ForceUpdateColor();
}

void euiSolidEntity::NotifyPointerEnter()
{
	ForceUpdateColor();
	euiEntity::NotifyPointerEnter();
}

void euiSolidEntity::NotifyPointerExit()
{
	ForceUpdateColor();
	euiEntity::NotifyPointerExit();
}

void euiSolidEntity::NotifyPointerPressed()
{
	ForceUpdateColor();
	euiEntity::NotifyPointerPressed();
}

void euiSolidEntity::NotifyPointerUnpressed()
{
	ForceUpdateColor();
	euiEntity::NotifyPointerUnpressed();
}


euiImageEntity::euiImageEntity(float _x, float _y, float _Width, float _Height, euiAnchor _Anchor, euiRenderer* Renderer, const std::string& ImagePath)
	: euiEntity(_x, _y, _Width, _Height, _Anchor)
	, Texture(nullptr)
{
	SetPosition(_x, _y);

	float positions[] = {
		 0.0f,		    0.0f,			0.0f, 0.0f,   //Bottom left
		 0.0f + _Width, 0.0f,			1.0f, 0.0f,   //Bottom right
		 0.0f + _Width, 0.0f + _Height, 1.0f, 1.0f,   //Top right
		 0.0f,		    0.0f + _Height, 0.0f, 1.0f    //Top left
	};

	uint indicies[] = {
		0, 1, 2,
		2, 3, 0
	};

	VertexBuffer = new euiVertexBuffer(positions, 4 * 4 * sizeof(float));

	Layout = new euiVertexBufferLayout();
	Layout->push<float>(2);
	Layout->push<float>(2);

	VertexArray = new euiVertexArray();
	VertexArray->AddBuffer(*VertexBuffer, *Layout);

	IndexBuffer = new euiIndexBuffer(indicies, 6);
	IndexBuffer->InitializeIndexBuffer(indicies, 6);

	Shader = new euiShader("./Resources/image.vshader", "./Resources/image.fshader");
	Shader->Bind();

	Texture = new euiTexture(ImagePath);
	Texture->Bind(Renderer);
	Shader->SetUniform1i("u_texture", Texture->GetSlot());

	VertexArray->Unbind();
	Shader->Unbind();
	VertexBuffer->Unbind();
	IndexBuffer->Unbind();
}

void euiImageEntity::Draw(class euiRenderer& Renderer)
{
	Renderer.Draw(this, Texture);
}