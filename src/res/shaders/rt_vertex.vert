#version 460

layout (location = 0) in vec3 vertPos;
layout (location = 1) in vec2 texCoords;

out vec2 accumTexCoords;

void main() {
    accumTexCoords = texCoords;
    gl_Position = vec4(vertPos, 1.0);
}