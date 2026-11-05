#pragma once

#include <glm/glm.hpp>
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#define CHECK_GL_ERRORS() Core::CheckOpenGLErrors()
#define LOG_ERROR(msg, ...) Core::WriteError(msg, ##__VA_ARGS__)

using Vector2f = glm::vec2;
using Vector3f = glm::vec3;
using Vector4f = glm::vec4;

using Rgb = Vector3f;
using Rgba = Vector4f;

using Matrix3f = glm::mat3;
using Matrix4f = glm::mat4;

using ResourceHandle = GLuint;

namespace Core
{
    void CheckOpenGLErrors();
    void WriteError(const char *msg, ...);
}
