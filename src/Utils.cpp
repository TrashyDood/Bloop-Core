#include "Utils.h"

#include <glad/gl.h>

#include <cstdio>

void Utils::CheckOpenGLErrors()
{
    for (GLenum error = glGetError(); error != GL_NO_ERROR; error = glGetError())
        std::fprintf(stderr, "OpenGL error: 0x%X\n", error);
}