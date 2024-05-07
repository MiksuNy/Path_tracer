#version 150

in vec3 vertPos;
in vec2 texCoords;

out vec2 accumTexCoords;

void main() {
    accumTexCoords = texCoords;
    gl_Position = vec4(vertPos, 1.0);
}