#include "Graphics.h"
#include "LogHandler.h"

#include <cinttypes>
#include <cmath>
#include <cstddef>
#include <limits>
#include <string>
#include <vector>

void Graphics::CheckGLErrors()
{
    LOG_SCOPE;

    for (GLenum error = glGetError(); error != GL_NO_ERROR; error = glGetError())
        LOG_ERROR("OpenGL error: 0x%X\n", error);
}

Matrix4f Graphics::GetMatrix_Perspective(const float fieldOfView,
    const float aspectRatio, const float nearPlane, const float farPlane)
{
    return glm::perspective(glm::radians(fieldOfView), aspectRatio, nearPlane, farPlane);
}

Matrix4f Graphics::GetMatrix_Orthographic(const float orthographicSize,
    const float aspectRatio, const float nearPlane, const float farPlane)
{
    const float halfHeight = orthographicSize * 0.5f;
    const float halfWidth = halfHeight * aspectRatio;
    return glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, nearPlane, farPlane);
}