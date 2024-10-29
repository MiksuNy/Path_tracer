#version 460

in vec2 accumTexCoords;

out vec4 fragColor;

uniform sampler2D accumTexture;

void main() {
	fragColor = vec4(texture(accumTexture, accumTexCoords).rgb, 1.0);
}