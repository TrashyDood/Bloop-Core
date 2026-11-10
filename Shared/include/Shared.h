#pragma once

#include <chrono>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
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
using Quaternionf = glm::quat;
using Matrix4f = glm::mat4;
using TimeDuration = std::chrono::duration<double>;

namespace Utils
{
    void CheckOpenGLErrors();
    void WriteError(const char *msg, ...);
}

namespace Shared
{
    [[nodiscard]] float GetWindowAspectRatio(GLFWwindow &window);
    int SetupWindow(GLFWwindow *&outWindow, const int width = 800, const int height = 600, const char *title = "Engine Window");

    template <typename T>
    bool GetWindowUserPointer(GLFWwindow &window, T *&outPointer)
    {
        outPointer = static_cast<T *>(glfwGetWindowUserPointer(&window));
        return outPointer != nullptr;
    }
}

namespace Colors
{
    inline constexpr Color4f Black{0.f, 0.f, 0.f, 1.f};
    inline constexpr Color4f White{1.f, 1.f, 1.f, 1.f};
    inline constexpr Color4f Red{1.f, 0.f, 0.f, 1.f};
    inline constexpr Color4f Green{0.f, 1.f, 0.f, 1.f};
    inline constexpr Color4f Blue{0.f, 0.f, 1.f, 1.f};
    inline constexpr Color4f Yellow{1.f, 1.f, 0.f, 1.f};
    inline constexpr Color4f Cyan{0.f, 1.f, 1.f, 1.f};
    inline constexpr Color4f Magenta{1.f, 0.f, 1.f, 1.f};
    inline constexpr Color4f Gray{0.5f, 0.5f, 0.5f, 1.f};
    inline constexpr Color4f Orange{1.f, 0.5f, 0.f, 1.f};
    inline constexpr Color4f Purple{0.5f, 0.f, 0.5f, 1.f};
    inline constexpr Color4f Transparent{0.f, 0.f, 0.f, 0.f};
}

namespace Vectors
{
    inline constexpr Vector3f v3fZero{0.f, 0.f, 0.f};
    inline constexpr Vector3f v3fOne{1.f, 1.f, 1.f};
    inline constexpr Vector3f v3fUp{0.f, -1.f, 0.f};
    inline constexpr Vector3f v3fDown{0.f, 1.f, 0.f};
    inline constexpr Vector3f v3fLeft{-1.f, 0.f, 0.f};
    inline constexpr Vector3f v3fRight{1.f, 0.f, 0.f};
    inline constexpr Vector3f v3fForward{0.f, 0.f, -1.f};
    inline constexpr Vector3f v3fBackward{0.f, 0.f, 1.f};

    inline constexpr Vector4f v4fZero{0.f, 0.f, 0.f, 0.f};
    inline constexpr Vector4f v4fOne{1.f, 1.f, 1.f, 1.f};
    inline constexpr Vector4f v4fUp{0.f, -1.f, 0.f, 0.f};
    inline constexpr Vector4f v4fDown{0.f, 1.f, 0.f, 0.f};
    inline constexpr Vector4f v4fLeft{-1.f, 0.f, 0.f, 0.f};
    inline constexpr Vector4f v4fRight{1.f, 0.f, 0.f, 0.f};
    inline constexpr Vector4f v4fForward{0.f, 0.f, -1.f, 0.f};
    inline constexpr Vector4f v4fBackward{0.f, 0.f, 1.f, 0.f};
}

namespace Quaternions
{
    inline constexpr Quaternionf qfIdentity{1.f, 0.f, 0.f, 0.f};
}
