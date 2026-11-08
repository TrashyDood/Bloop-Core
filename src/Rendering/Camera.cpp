#include "Rendering/Camera.h"

Matrix4f Rendering::Camera::GetViewMatrix() const
{
    const Vector3f &position = transform.GetPosition();
    return glm::lookAt(position, position + transform.GetForward(), transform.GetUp());
}

Matrix4f Rendering::Camera::GetOrthoProjectionMatrix(const float halfWidth, const float halfHeight) const
{
    return glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, nearPlane, farPlane);
}

Matrix4f Rendering::Camera::GetPerspProjectionMatrix(const float aspectRatio) const
{
    return glm::perspective(glm::radians(fieldOfView), aspectRatio, nearPlane, farPlane);
}

Matrix4f Rendering::Camera::GetProjectionMatrix(const float aspectRatio) const
{
    if (isOrthographic)
    {
        const float halfHeight = orthographicSize * 0.5f;
        const float halfWidth = halfHeight * aspectRatio;
        return GetOrthoProjectionMatrix(halfWidth, halfHeight);
    }
    return GetPerspProjectionMatrix(aspectRatio);
}
