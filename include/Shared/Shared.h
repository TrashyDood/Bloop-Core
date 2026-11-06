#pragma once

#include <glm/glm.hpp>
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#define CHECK_GL_ERRORS() Utils::CheckOpenGLErrors()
#define LOG_ERROR(msg, ...) Utils::WriteError(msg, ##__VA_ARGS__)

using Vector2f = glm::vec2;
using Vector3f = glm::vec3;
using Vector4f = glm::vec4;

using Color3 = Vector3f;
using Color4 = Vector4f;

using Matrix3f = glm::mat3;
using Matrix4f = glm::mat4;

using ResourceHandle = GLuint;

namespace Utils
{
    void CheckOpenGLErrors();
    void WriteError(const char *msg, ...);
}

namespace Colors
{
    inline const Color4 Black{0.0f, 0.0f, 0.0f, 1.0f};
    inline const Color4 White{1.0f, 1.0f, 1.0f, 1.0f};
    inline const Color4 Red{1.0f, 0.0f, 0.0f, 1.0f};
    inline const Color4 Green{0.0f, 1.0f, 0.0f, 1.0f};
    inline const Color4 Blue{0.0f, 0.0f, 1.0f, 1.0f};
    inline const Color4 Yellow{1.0f, 1.0f, 0.0f, 1.0f};
    inline const Color4 Cyan{0.0f, 1.0f, 1.0f, 1.0f};
    inline const Color4 Magenta{1.0f, 0.0f, 1.0f, 1.0f};
    inline const Color4 Gray{0.5f, 0.5f, 0.5f, 1.0f};
    inline const Color4 Orange{1.0f, 0.5f, 0.0f, 1.0f};
    inline const Color4 Purple{0.5f, 0.0f, 0.5f, 1.0f};
    inline const Color4 Transparent{0.0f, 0.0f, 0.0f, 0.0f};
}
