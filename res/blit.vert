#version 330

layout (location = 0) in vec2 inPosition;
layout (location = 1) in vec2 inCoords;

out vec2 coords;

void main() {
	gl_Position = vec4(inPosition, 0, 1);
    coords = inCoords;
}
