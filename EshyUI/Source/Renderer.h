
#pragma once

#include "Util.h"

#include "glm/glm.hpp"

#include <GLFW/glfw3.h>

#include <string>
#include <map>

class euiRenderer
{
public:

	euiRenderer(GLFWwindow* _Window);

	void Clear() const;
	void SetBackgroundColor(glm::vec4 Color);
	void Draw(const class euiVertexArray& va, const class euiIndexBuffer& ib, class euiShader& shader);
	void Draw(class euiEntity* Entity, const glm::vec4 Color);
	void Draw(class euiEntity* Entity, class euiTexture* Texture = nullptr);

	void UpdateScreenSize(float Width, float Height);

	void InformBindTexture(class euiTexture* Texture, uint Slot);
	int GetFirstUnboundTextureSlot();

	GLFWwindow* GetWindow() {return Window;}
	float GetWindowWidth() const {return WindowWidth;}
	float GetWindowHeight() const {return WindowHeight;}
	glm::mat4 GetProjectionMatrix() const {return ProjectionMatrix;}
	std::map<uint, class euiTexture*> GetTextureBindingInfo() {return TextureBindingInfo;}

private:

	GLFWwindow* Window;
	float WindowWidth;
	float WindowHeight;
	glm::mat4 ProjectionMatrix;
	std::map<uint, class euiTexture*> TextureBindingInfo;
};

