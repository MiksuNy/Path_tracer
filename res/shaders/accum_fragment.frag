#version 460

in vec2 accumTexCoords;
out vec4 fragColor;

uniform sampler2D accumTexture;

//vec4 DownSampleFrame(sampler2D uniformSampler, float offset)
//{
//    vec2 pixelOffset = vec2(1.0f/textureSize(uniformSampler, 0).x, 1.0f/textureSize(uniformSampler, 0).y);
//
//    vec3 downScaleColor = vec3(0.0f);
//    {
//        float Pixels = offset;
//        float dx = (1.0 / Pixels);
//        float dy = (textureSize(uniformSampler, 0).x / textureSize(uniformSampler, 0).y / Pixels);
//        vec2 Coord = vec2(dx * floor(accumTexCoords.s / dx),
//                          dy * floor(accumTexCoords.t / dy));
//
//        downScaleColor += texture(uniformSampler, vec2(Coord.x - -pixelOffset.x, Coord.y)).xyz;
//        downScaleColor += texture(uniformSampler, vec2(Coord.x + -pixelOffset.x, Coord.y)).xyz;
//        downScaleColor += texture(uniformSampler, vec2(Coord.x, Coord.y - pixelOffset.y)).xyz;
//        downScaleColor += texture(uniformSampler, vec2(Coord.x, Coord.y + pixelOffset.y)).xyz;
//	    downScaleColor *= 0.25f;
//    }
//    return (vec4(downScaleColor, 1.0f));
//}

void main() {
	fragColor = texture(accumTexture, accumTexCoords);
}