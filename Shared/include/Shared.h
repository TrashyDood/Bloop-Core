#pragma once

#include "IOUtils.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <regex>
#include <stdexcept>
#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#define SMALL_NUMBER    1e-6f
#define BIG_NUMBER      1e+6f

using namespace std::chrono_literals;

using Vector2f =        glm::vec2;
using Vector3f =        glm::vec3;
using Vector4f =        glm::vec4;

using Vector2i =        glm::ivec2;
using Vector3i =        glm::ivec3;
using Vector4i =        glm::ivec4;

using Color3f =         Vector3f;
using Color4f =         Vector4f;

using Matrix3f =        glm::mat3;
using Matrix4f =        glm::mat4;

using Quaternion =     glm::quat;

using Int8 =            std::int8_t;
using Int16 =           std::int16_t;
using Int32 =           std::int32_t;
using Int64 =           std::int64_t;
using UInt8 =           std::uint8_t;
using UInt16 =          std::uint16_t;
using UInt32 =          std::uint32_t;
using UInt64 =          std::uint64_t;

using TimeDuration =    std::chrono::duration<double>;

template <glm::length_t TLength, typename T>
struct Box
{
    glm::vec<TLength, T> position,
        dimensions;

    Box() = default;

    [[nodiscard]] T Area() const noexcept { return dimensions.x * dimensions.y; }
    [[nodiscard]] T Width() const noexcept { return dimensions.x; }
    [[nodiscard]] T Height() const noexcept { return dimensions.y; }
    [[nodiscard]] T Size() const noexcept { return static_cast<T>(glm::length(glm::vec<TLength, float>(dimensions))); }
    [[nodiscard]] T X() const noexcept { return position.x; }
    [[nodiscard]] T Y() const noexcept { return position.y; }
    [[nodiscard]] glm::vec<TLength, T> Center() const noexcept
    {
        return position + dimensions / static_cast<T>(2);
    }
};

using Box2i = Box<2, Int32>;
using Box2f = Box<2, float>;
using Box3i = Box<3, Int32>;
using Box3f = Box<3, float>;

// Measured in hertz (Hz), which is cycles per second.
inline constexpr TimeDuration GetFrequency(const double frequency) { return 1.0s / frequency; }

namespace Colors
{
    inline constexpr Color4f Black          {0.f, 0.f, 0.f, 1.f};
    inline constexpr Color4f White          {1.f, 1.f, 1.f, 1.f};
    inline constexpr Color4f Red            {1.f, 0.f, 0.f, 1.f};
    inline constexpr Color4f Green          {0.f, 1.f, 0.f, 1.f};
    inline constexpr Color4f Blue           {0.f, 0.f, 1.f, 1.f};
    inline constexpr Color4f Yellow         {1.f, 1.f, 0.f, 1.f};
    inline constexpr Color4f Cyan           {0.f, 1.f, 1.f, 1.f};
    inline constexpr Color4f Magenta        {1.f, 0.f, 1.f, 1.f};
    inline constexpr Color4f Gray           {0.5f, 0.5f, 0.5f, 1.f};
    inline constexpr Color4f Orange         {1.f, 0.5f, 0.f, 1.f};
    inline constexpr Color4f Purple         {0.5f, 0.f, 0.5f, 1.f};
    inline constexpr Color4f Transparent    {0.f, 0.f, 0.f, 0.f};
}

namespace Vectors
{
    inline constexpr Vector3f v3fZero       {0.f, 0.f, 0.f};
    inline constexpr Vector3f v3fOne        {1.f, 1.f, 1.f};
    inline constexpr Vector3f v3fUp         {0.f, 1.f, 0.f};
    inline constexpr Vector3f v3fDown       {0.f, -1.f, 0.f};
    inline constexpr Vector3f v3fLeft       {-1.f, 0.f, 0.f};
    inline constexpr Vector3f v3fRight      {1.f, 0.f, 0.f};
    inline constexpr Vector3f v3fForward    {0.f, 0.f, -1.f};
    inline constexpr Vector3f v3fBackward   {0.f, 0.f, 1.f};

    inline constexpr Vector4f v4fZero       {0.f, 0.f, 0.f, 0.f};
    inline constexpr Vector4f v4fOne        {1.f, 1.f, 1.f, 1.f};
    inline constexpr Vector4f v4fUp         {0.f, 1.f, 0.f, 0.f};
    inline constexpr Vector4f v4fDown       {0.f, -1.f, 0.f, 0.f};
    inline constexpr Vector4f v4fLeft       {-1.f, 0.f, 0.f, 0.f};
    inline constexpr Vector4f v4fRight      {1.f, 0.f, 0.f, 0.f};
    inline constexpr Vector4f v4fForward    {0.f, 0.f, -1.f, 0.f};
}

namespace Quaternions
{
    inline constexpr Quaternion qfIdentity{1.f, 0.f, 0.f, 0.f};
}