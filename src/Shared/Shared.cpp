#include "Shared/Shared.h"

#include <glad/gl.h>

#include <cstdarg>
#include <cstdio>

void Utils::CheckOpenGLErrors()
{
    for (GLenum error = glGetError(); error != GL_NO_ERROR; error = glGetError())
        std::fprintf(stderr, "OpenGL error: 0x%X\n", error);
}

void Utils::WriteError(const char *msg, ...)
{
    va_list args;
    va_start(args, msg);
    std::vfprintf(stderr, msg, args);
    va_end(args);
}
