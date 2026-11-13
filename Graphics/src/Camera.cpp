#include "Camera.h"

Matrix4f Graphics::Camera::GetOrthographicMatrix(const float aspectRatio) const
{
    const float halfHeight = orthographicSize * 0.5f;
    const float halfWidth = halfHeight * aspectRatio;
    return glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, nearPlane, farPlane);
}

Matrix4f Graphics::Camera::GetPerspectiveMatrix(const float aspectRatio) const
{
    return glm::perspective(glm::radians(fieldOfView), aspectRatio, nearPlane, farPlane);
}

Matrix4f Graphics::Camera::GetProjectionMatrix(const float aspectRatio) const
{
    return isOrthographic ? GetOrthographicMatrix(aspectRatio) : GetPerspectiveMatrix(aspectRatio);
}
