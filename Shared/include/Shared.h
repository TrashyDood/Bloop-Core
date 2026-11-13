#pragma once

#include "InputCodes.h"

#include <chrono>
#include <stdexcept>
#include <stddef.h>
#include <memory>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define SHARED_DEFINED_WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define SHARED_DEFINED_NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>
#ifdef SHARED_DEFINED_NOMINMAX
#undef NOMINMAX
#undef SHARED_DEFINED_NOMINMAX
#endif
#ifdef SHARED_DEFINED_WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#undef SHARED_DEFINED_WIN32_LEAN_AND_MEAN
#endif

struct TimerResolution
{
    bool active;

    explicit TimerResolution(bool enabled) : active(enabled)
    {
        if (active && timeBeginPeriod(1) != TIMERR_NOERROR)
            throw std::runtime_error("Failed to increase Windows timer resolution.");
    }

    ~TimerResolution()
    {
        if (active)
            timeEndPeriod(1);
    }
};
#endif

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#define LOG_ERROR(msg, ...) WriteError(msg, ##__VA_ARGS__)
#define SMALL_NUMBER 1e-6f

using namespace std::chrono_literals;

template<typename T>
using SharedList = std::vector<std::shared_ptr<T>>;
using Vector2f = glm::vec2;
using Vector3f = glm::vec3;
using Vector4f = glm::vec4;
using Color3f = Vector3f;
using Color4f = Vector4f;
using Matrix3f = glm::mat3;
using Quaternionf = glm::quat;
using Matrix4f = glm::mat4;
using TimeDuration = std::chrono::duration<double>;

void WriteError(const char *msg, ...);

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