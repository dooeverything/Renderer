#version 450 core
layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_tangent;
layout (location = 3) in vec2 in_texCoord;

out vec2 frag_texCoord;

void main()
{
    frag_texCoord = in_texCoord;
    gl_Position = vec4(in_pos, 1.0);
}