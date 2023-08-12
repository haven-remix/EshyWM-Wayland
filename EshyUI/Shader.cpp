
#include "Shader.h"

#include <GL/glew.h>

#include <iostream>
#include <fstream>
#include <sstream>

static std::string ParseShaderFromFile(const std::string& FilePath)
{
	std::ifstream stream(FilePath);
	std::string line;
	std::stringstream ss;

	while (getline(stream, line))
		ss << line << "\n";

	return ss.str();
}

euiShader::euiShader(const std::string& _VertexShaderSource, const std::string& _FragmentShaderSource)
	: VertexShaderSource(_VertexShaderSource)
	, FragmentShaderSource(_FragmentShaderSource)
{
	InitializeShader(_VertexShaderSource, _FragmentShaderSource);
}

euiShader::~euiShader()
{
	glDeleteProgram(RendererID);
}

void euiShader::Bind() const
{
	glUseProgram(RendererID);
}

void euiShader::Unbind() const
{
	glUseProgram(0);
}

void euiShader::InitializeShader(const std::string& VertexShaderSource, const std::string& FragmentShaderSource)
{
	const std::string VertexShader = ParseShaderFromFile(VertexShaderSource);
	const std::string FragmentShader = ParseShaderFromFile(FragmentShaderSource);
	RendererID = CreateShader(VertexShader, FragmentShader);
}

void euiShader::SetUniform1i(const std::string& name, int v0)
{
	glUniform1i(GetUniformLocation(name), v0);
}

void euiShader::SetUniform1f(const std::string& name, float v0)
{
	glUniform1f(GetUniformLocation(name), v0);
}

void euiShader::SetUniform4f(const std::string& name, float v0, float v1, float v2, float v3)
{
	glUniform4f(GetUniformLocation(name), v0, v1, v2, v3);
}

void euiShader::SetUniformMat4f(const std::string& name, const glm::mat4& matrix)
{
	glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, &matrix[0][0]);
}

uint euiShader::CreateShader(const std::string& vertex_shader, const std::string& fragment_shader)
{
	uint program = glCreateProgram();
	uint vs = CompileShader(GL_VERTEX_SHADER, vertex_shader);
	uint fs = CompileShader(GL_FRAGMENT_SHADER, fragment_shader);

	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);
	glValidateProgram(program);
	glDeleteShader(vs);
	glDeleteShader(fs);

	return program;
}

uint euiShader::CompileShader(uint type, const std::string& source)
{
	uint id = glCreateShader(type);
	const char* src = source.c_str();
	glShaderSource(id, 1, &src, nullptr);
	glCompileShader(id);

	int result;
	glGetShaderiv(id, GL_COMPILE_STATUS, &result);

	if (result == GL_FALSE)
	{
		int length;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
		char* message = (char*)alloca(length * sizeof(char));
		glGetShaderInfoLog(id, length, &length, message);
		std::cout << "Failed to compile " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << "shader!" << std::endl;
		std::cout << message << std::endl;
		glDeleteShader(id);
		return 0;
	}

	return id;
}

int euiShader::GetUniformLocation(const std::string& name)
{
	return glGetUniformLocation(RendererID, name.c_str());
}
