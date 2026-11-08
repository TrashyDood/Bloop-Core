#include "Rendering/Camera.h"

Vector3f Rendering::Camera::GetDirection(const Vector3f &localDirection) const
{
    const Vector4f direction = transform.GetRotationMatrix() * Vector4f{localDirection, 0.0f};
    return glm::normalize(Vector3f{direction});
}

Matrix4f Rendering::Camera::GetViewMatrix() const
{
    const Vector3f front = GetDirection({0.0f, 0.0f, -1.0f});
    const Vector3f upDirection = GetDirection({0.0f, 1.0f, 0.0f});
    const Vector3f &position = transform.GetPosition();
    return glm::lookAt(position, position + front, upDirection);
}

Matrix4f Rendering::Camera::GetProjectionMatrix(float aspectRatio) const
{
    return glm::perspective(glm::radians(fieldOfView), aspectRatio, nearPlane, farPlane);
}
