#version 450
layout(std140, binding = 0) uniform Input
{
    vec3 color;
    float size;
    mat4 mvp;
};

layout(location = 0) in vec2 aPosition;

out vec3 oColor;

void main()
{
    gl_Position = mvp * vec4(aPosition, 0.0, 1.0);
    gl_PointSize = size;
    oColor = color.xyz;
}
