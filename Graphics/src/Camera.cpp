#include "Camera.h"

#pragma region Camera class
Matrix4f Graphics::Camera::GetOrthographicMatrix(const float aspectRatio) const
{
    const float halfHeight = orthographicSize * 0.5f;
    const float halfWidth = halfHeight * aspectRatio;
    return glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, nearPlane, farPlane);
}

Matrix4f Graphics::Camera::GetPerspectiveMatrix(const float aspectRatio) const
{
    return Graphics::GetPerspectiveMatrix(fieldOfView, aspectRatio, nearPlane, farPlane);
}

void Graphics::Camera::SetPerspective(const float newFieldOfView,
    const float newNearPlane, const float newFarPlane, const bool newIsOrthographic)
{
    fieldOfView = newFieldOfView;
    nearPlane = newNearPlane;
    farPlane = newFarPlane;
    isOrthographic = newIsOrthographic;
}

Matrix4f Graphics::Camera::GetProjectionMatrix(const float aspectRatio) const
{
    return isOrthographic ?
        GetOrthographicMatrix(aspectRatio) :
        GetPerspectiveMatrix(aspectRatio);
}
#pragma endregion

Matrix4f Graphics::GetPerspectiveMatrix(const float fieldOfView,
    const float aspectRatio, const float nearPlane, const float farPlane)
{
    return glm::perspective(glm::radians(fieldOfView), aspectRatio, nearPlane, farPlane);
}

Matrix4f Graphics::GetOrthographicMatrix(const float orthographicSize, const float aspectRatio,
    const float nearPlane, const float farPlane)
{
    const float halfHeight = orthographicSize * 0.5f;
    const float halfWidth = halfHeight * aspectRatio;
    return glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, nearPlane, farPlane);
}
