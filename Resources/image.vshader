#version 330 core

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 tex_coord;

out vec2 v_tex_coord;

uniform mat4 u_mvp;

void main()
{
	gl_Position = u_mvp * position;
	v_tex_coord = tex_coord;
}