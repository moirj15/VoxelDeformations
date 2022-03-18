#version 450
layout(std140, binding = 0) uniform Input
{
    vec4 color; // w component for padding
    mat4 mvp;
};

layout(location = 0) in vec2 aPosition;

out vec3 oColor;

void main()
{
    gl_Position = mvp * vec4(aPosition, 0.0, 1.0);
    oColor = color.xyz;
}
