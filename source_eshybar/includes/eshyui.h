
#pragma once

#include "glm/glm.hpp"

#include <GLFW/glfw3.h>

#include <sys/types.h>

#include <string>
#include <vector>

namespace eshyui
{
class shader;
class vertex_array;
class vertex_buffer;
class vertex_buffer_layout;
class index_buffer;

class renderer
{
public:

    void clear() const;
    void draw(const vertex_array& va, const index_buffer& ib, const shader& shader);
};

class texture
{
public:

    texture(const std::string& path);
    ~texture();

    void bind(uint slot = 0) const;
    void unbind() const;

    inline int get_width() const {return width;}
    inline int get_height() const {return height;}

private:

    uint renderer_id;
    std::string file_path;
    unsigned char* local_buffer;
    int width;
    int height;
    int bpp;
};

class shader
{
public:

    shader(const std::string& vertex_shader_source, const std::string& fragment_shader_source);
    ~shader();

    void bind() const;
    void unbind() const;

    void set_uniform_1i(const std::string& name, int v0);
    void set_uniform_1f(const std::string& name, float v0);
    void set_uniform_4f(const std::string& name, float v0, float v1, float v2, float v3);
    void set_uniform_mat4f(const std::string& name, const glm::mat4& matrix);

private:

    uint renderer_id;
    std::string vertex_shader_source;
    std::string fragment_shader_source;

    uint create_shader(const std::string& vertex_shader, const std::string& fragment_shader);
    uint compile_shader(uint type, const std::string& source);
    uint get_uniform_location(const std::string& name);
};

class vertex_array
{
public:

    vertex_array();
    ~vertex_array();

    void bind() const;
    void unbind() const;

    void add_buffer(const vertex_buffer& vb, const vertex_buffer_layout& vbl);

private:

    uint renderer_id;
};

struct vertex_buffer_element
{
    uint type;
    uint count;
    unsigned char normalized;
};

class vertex_buffer_layout
{
public:

    vertex_buffer_layout()
        : stride(0)
    {}

    template<typename T>
    void push(uint count)
    {
        static_assert(false);
    }

    inline const std::vector<vertex_buffer_element>& get_elements() const {return elements;}
    inline uint get_stride() const {return stride;}

private:

    std::vector<vertex_buffer_element> elements;
    uint stride;
};

template<>
inline void vertex_buffer_layout::push<float>(uint count)
{
    elements.push_back({GL_FLOAT, count, false});
    stride += count * sizeof(float);
}

template<>
inline void vertex_buffer_layout::push<uint>(uint count)
{
    elements.push_back({GL_UNSIGNED_INT, count, false});
    stride += count * sizeof(uint);
}

template<>
inline void vertex_buffer_layout::push<unsigned char>(uint count)
{
    elements.push_back({GL_UNSIGNED_BYTE, count, true});
    stride += count * sizeof(unsigned char);
}

class vertex_buffer
{
public:

    vertex_buffer(const void* data, uint size);
    ~vertex_buffer();

    void bind() const;
    void unbind() const;

private:

    uint renderer_id;
};

class index_buffer
{
public:

    index_buffer(const uint* data, uint count);
    ~index_buffer();

    void bind() const;
    void unbind() const;

    inline uint get_count() const {return count;}

private:

    uint renderer_id;
    uint count;
};

GLFWwindow* init_eshyui();
void shutdown();

uint create_shader(const std::string& vertex_shader, const std::string& fragment_shader);

uint rect(int x, int y, int width, int height);
}