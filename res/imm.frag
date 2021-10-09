#version 330

in vec4 color;
in vec2 coords;
in float factor;

out vec4 outColor;

uniform sampler2D tex;

void main() {
    outColor = mix(color, color * texture(tex, coords), factor);
    float gamma = 2.2;
    outColor = vec4(pow(outColor.rgb, vec3(gamma)), outColor.a);
}