#pragma once

#include "Shared.h"

#include <cstdio>
#include <cstddef>
#include <string>
#include <string_view>
#include <filesystem>

#include <glad/gl.h>

namespace Graphics
{
    using ResourceHandle = GLuint;

    inline constexpr float DefaultAspectRatio = 16.0f / 9.0f;

    void CheckGLErrors();

    [[nodiscard]]
    Matrix4f GetMatrix_Perspective(const float fieldOfView,
        const float aspectRatio, const float nearPlane, const float farPlane);
    [[nodiscard]]
    Matrix4f GetMatrix_Orthographic(const float orthographicSize,
        const float aspectRatio, const float nearPlane, const float farPlane);
}