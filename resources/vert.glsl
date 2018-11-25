#version  450 core
layout(location = 0) in vec4 vertPos;
layout(location = 2) in vec2 vertTex;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform vec2 iResolution;
out vec2 fragCoord;

void main()
{
    gl_Position = vertPos;
    fragCoord = vertTex * iResolution;
}
