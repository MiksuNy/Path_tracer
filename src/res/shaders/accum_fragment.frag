#version 460

in vec2 accumTexCoords;
out vec4 fragColor;

uniform sampler2D accumTexture;

void main() {
	fragColor = texture(accumTexture, accumTexCoords);
}