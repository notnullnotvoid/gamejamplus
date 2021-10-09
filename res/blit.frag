#version 330

in vec2 coords;

out vec4 outColor;

uniform sampler2D tex;
uniform vec2 texSize;
uniform float scale;

void main() {
	//credit to d7samurai for this excellent pixel smoothing algorithm
	vec2 pixel = coords * texSize;
    vec2 uv = floor(pixel) + 0.5;
    uv += 1.0 - clamp((1.0 - fract(pixel)) * scale, 0.0, 1.0);
    outColor = texture(tex, uv / texSize);
}
