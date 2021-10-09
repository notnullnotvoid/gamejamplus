#ifndef GLUTIL_HPP
#define GLUTIL_HPP

#define GL_SILENCE_DEPRECATION
#include "glad.h"
#include "types.hpp"

struct Texture {
	uint handle;
	int width;
	int height;
};

Texture load_texture(const char * imagePath);
GLuint create_program_from_files(const char * vertexShaderPath, const char * fragmentShaderPath);
void gl_error(const char * when);

#endif //GLUTIL_HPP
