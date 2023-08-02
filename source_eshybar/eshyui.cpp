
#include <GL/glew.h>

#include "eshyui.h"

#include "stb_image.h"

#include <iostream>

#define eshyui_glfw_check(condition)       if (!(condition)) {glfwTerminate(); return nullptr;}

static uint compile_shader(uint type, const std::string& source)
{
    uint id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);

    if(result == GL_FALSE)
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

namespace eshyui
{
void renderer::clear() const
{
    glClear(GL_COLOR_BUFFER_BIT);
}

void renderer::draw(const vertex_array& va, const index_buffer& ib, const shader& shader)
{
    shader.bind();
	va.bind();
	ib.bind();

    glDrawElements(GL_TRIANGLES, ib.get_count(), GL_UNSIGNED_INT, nullptr);
}


texture::texture(const std::string& path)
    : file_path(path)
    , local_buffer(nullptr)
    , bpp(0)
{
    stbi_set_flip_vertically_on_load(1);
    local_buffer = stbi_load(path.c_str(), &width, &height, &bpp, 4);

    glGenTextures(1, &renderer_id);
    glBindTexture(GL_TEXTURE_2D, renderer_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, local_buffer);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (local_buffer)
        stbi_image_free(local_buffer);
}

texture::~texture()
{
    glDeleteTextures(1, &renderer_id);
}

void texture::bind(uint slot) const
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, renderer_id);
}

void texture::unbind() const
{
    glBindTexture(GL_TEXTURE_2D, 0);
}


shader::shader(const std::string& _vertex_shader_source, const std::string& _fragment_shader_source)
    : vertex_shader_source(_vertex_shader_source)
    , fragment_shader_source(_fragment_shader_source)
{
    renderer_id = create_shader(_vertex_shader_source, _fragment_shader_source);
}

shader::~shader()
{
    glDeleteProgram(renderer_id);
}

void shader::bind() const
{
	glUseProgram(renderer_id);
}

void shader::unbind() const
{
    glUseProgram(0);
}

uint shader::create_shader(const std::string& vertex_shader, const std::string& fragment_shader)
{
    uint program = glCreateProgram();
    uint vs = compile_shader(GL_VERTEX_SHADER, vertex_shader);
    uint fs = compile_shader(GL_FRAGMENT_SHADER, fragment_shader);

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glValidateProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

uint shader::compile_shader(uint type, const std::string& source)
{
    uint id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);

    if(result == GL_FALSE)
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

void shader::set_uniform_1i(const std::string& name, int v0)
{
    int location = glGetUniformLocation(renderer_id, name.c_str());
	glUniform1i(location, v0);
}

void shader::set_uniform_1f(const std::string& name, float v0)
{
    int location = glGetUniformLocation(renderer_id, name.c_str());
	glUniform1f(location, v0);
}

void shader::set_uniform_4f(const std::string& name, float v0, float v1, float v2, float v3)
{
    int location = glGetUniformLocation(renderer_id, name.c_str());
	glUniform4f(location, v0, v1, v2, v3);
}

void shader::set_uniform_mat4f(const std::string& name, const glm::mat4& matrix)
{
    int location = glGetUniformLocation(renderer_id, name.c_str());
	glUniformMatrix4fv(location, 1, GL_FALSE, &matrix[0][0]);
}


vertex_array::vertex_array()
{
    glGenVertexArrays(1, &renderer_id);
}

vertex_array::~vertex_array()
{
    glDeleteVertexArrays(1, &renderer_id);
}

void vertex_array::bind() const
{
    glBindVertexArray(renderer_id);
}

void vertex_array::unbind() const
{
    glBindVertexArray(0);
}

void vertex_array::add_buffer(const vertex_buffer& vb, const vertex_buffer_layout& vbl)
{
    bind();
    vb.bind();

    uint64_t offset = 0;
    for (uint i = 0; i < vbl.get_elements().size(); ++i)
    {
        glEnableVertexAttribArray(i);
	    glVertexAttribPointer(i, vbl.get_elements()[i].count, vbl.get_elements()[i].type, vbl.get_elements()[i].normalized, vbl.get_stride(), (const void*)offset);
        offset += vbl.get_elements()[i].count * sizeof(vbl.get_elements()[i].type);
    }
}


vertex_buffer::vertex_buffer(const void* data, uint size)
{
    glGenBuffers(1, &renderer_id);
	glBindBuffer(GL_ARRAY_BUFFER, renderer_id);
	glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
}

vertex_buffer::~vertex_buffer()
{
    glDeleteBuffers(1, &renderer_id);
}

void vertex_buffer::bind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, renderer_id);
}

void vertex_buffer::unbind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}


index_buffer::index_buffer(const uint* data, uint _count)
    : count(_count)
{
    glGenBuffers(1, &renderer_id);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer_id);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint), data, GL_STATIC_DRAW);
}

index_buffer::~index_buffer()
{
    glDeleteBuffers(1, &renderer_id);
}

void index_buffer::bind() const
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer_id);
}

void index_buffer::unbind() const
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


GLFWwindow* init_eshyui()
{
    if (!glfwInit())
        return nullptr;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(640, 480, "Eshybar", NULL, NULL);
    eshyui_glfw_check(window);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    eshyui_glfw_check(glewInit() == GLEW_OK);

#ifdef ESHYUI_PRINT_OPENGL_VERSION
    std::cout << "Version:" << glGetString(GL_VERSION)<< std::endl;
#endif

    return window;
}

void shutdown()
{
    glfwTerminate();
}


uint rect(int x, int y, int width, int height)
{
    float vertices[] = {
        -0.5f, -0.5f, 0.0f, // Bottom-left
        0.5f, -0.5f, 0.0f,  // Bottom-right
        0.5f, 0.5f, 0.0f,   // Top-right
        -0.5f, 0.5f, 0.0f   // Top-left
    };

    uint VBO;
    uint VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Specify the vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}
}