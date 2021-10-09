#version 330

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec4 inColor;
layout (location = 2) in vec2 inCoords;
layout (location = 3) in float inFactor;

out vec4 color;
out vec2 coords;
out float factor;

uniform mat4 transform;
uniform vec2 scale;

void main() {
    gl_Position = transform * vec4(inPosition, 1);
    gl_Position.y = -gl_Position.y;

    // float gamma = 2.2;
    // color = vec4(pow(inColor.rgb, vec3(gamma)), inColor.a);
    color = inColor;

    coords = inCoords * scale;
    factor = inFactor;
}