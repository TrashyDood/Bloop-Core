#pragma once

#include <chrono>

#include <glm/glm.hpp>
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#define CHECK_GL_ERRORS() Utils::CheckOpenGLErrors()
#define LOG_ERROR(msg, ...) Utils::WriteError(msg, ##__VA_ARGS__)

using namespace std::chrono_literals;

using Vector2f = glm::vec2;
using Vector3f = glm::vec3;
using Vector4f = glm::vec4;
using Color3f = Vector3f;
using Color4f = Vector4f;
using Matrix3f = glm::mat3;
using Matrix4f = glm::mat4;
using ResourceHandle = GLuint;
using TimeDuration = std::chrono::duration<double>;

namespace Utils
{
    void CheckOpenGLErrors();
    void WriteError(const char *msg, ...);
}

namespace Colors
{
    inline const Color4f Black{0.0f, 0.0f, 0.0f, 1.0f};
    inline const Color4f White{1.0f, 1.0f, 1.0f, 1.0f};
    inline const Color4f Red{1.0f, 0.0f, 0.0f, 1.0f};
    inline const Color4f Green{0.0f, 1.0f, 0.0f, 1.0f};
    inline const Color4f Blue{0.0f, 0.0f, 1.0f, 1.0f};
    inline const Color4f Yellow{1.0f, 1.0f, 0.0f, 1.0f};
    inline const Color4f Cyan{0.0f, 1.0f, 1.0f, 1.0f};
    inline const Color4f Magenta{1.0f, 0.0f, 1.0f, 1.0f};
    inline const Color4f Gray{0.5f, 0.5f, 0.5f, 1.0f};
    inline const Color4f Orange{1.0f, 0.5f, 0.0f, 1.0f};
    inline const Color4f Purple{0.5f, 0.0f, 0.5f, 1.0f};
    inline const Color4f Transparent{0.0f, 0.0f, 0.0f, 0.0f};
}
