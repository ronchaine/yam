#version 140
#extension GL_ARB_explicit_attrib_location : require

layout(location = 0) in vec2 in_pos;
layout(location = 2) in vec4 in_col;

out vec4 ex_col;

void main()
{
   ex_col = in_col;
   gl_Position = vec4(in_pos, 0.0, 1.0);
}
