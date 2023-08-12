
#pragma once

#include "Util.h"

#include "glm/glm.hpp"

#include <string>

class euiShader
{
public:

	euiShader() {}
	euiShader(const std::string& VertexShaderSource, const std::string& FragmentShaderSource);
	~euiShader();

	void Bind() const;
	void Unbind() const;

	void InitializeShader(const std::string& VertexShaderSource, const std::string& FragmentShaderSource);

	void SetUniform1i(const std::string& name, int v0);
	void SetUniform1f(const std::string& name, float v0);
	void SetUniform4f(const std::string& name, float v0, float v1, float v2, float v3);
	void SetUniformMat4f(const std::string& name, const glm::mat4& matrix);

private:

	uint RendererID;
	std::string VertexShaderSource;
	std::string FragmentShaderSource;

	uint CreateShader(const std::string& vertex_shader, const std::string& fragment_shader);
	uint CompileShader(uint type, const std::string& source);
	int GetUniformLocation(const std::string& name);
};

