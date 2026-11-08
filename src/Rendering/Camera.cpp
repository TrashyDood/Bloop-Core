#include "Rendering/Camera.h"

Matrix4f Rendering::Camera::GetViewMatrix() const
{
    const Matrix4f rotation = transform.GetRotationMatrix();
    const Vector3f &position = transform.GetPosition();
    const Vector3f forward = glm::normalize(Vector3f{rotation * Vectors::v4fForward});
    const Vector3f up = glm::normalize(Vector3f{rotation * Vectors::v4fDown});
    return glm::lookAt(position, position + forward, up);
}

Matrix4f Rendering::Camera::GetProjectionMatrix(float aspectRatio) const
{
    return glm::perspective(glm::radians(fieldOfView), aspectRatio, nearPlane, farPlane);
}
