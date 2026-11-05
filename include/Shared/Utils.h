#pragma once

#define CHECK_GL_ERRORS() Utils::CheckOpenGLErrors()
#define LOG_ERROR(msg, ...) Utils::WriteError(msg, ##__VA_ARGS__)

namespace Utils
{
    void CheckOpenGLErrors();
    void WriteError(const char *msg, ...);
}
