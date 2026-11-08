#include "Rendering/Camera.h"

Matrix4f Rendering::Camera::GetViewMatrix() const
{
    const Matrix4f rotation = transform.GetRotationMatrix();
    const Vector3f &position = transform.GetPosition();
    const Vector4f localForward{0.0f, 0.0f, -1.0f, 0.0f};
    const Vector4f localUp{0.0f, 1.0f, 0.0f, 0.0f};
    const Vector3f forward = glm::normalize(Vector3f{rotation * localForward});
    const Vector3f up = glm::normalize(Vector3f{rotation * localUp});
    return glm::lookAt(position, position + forward, up);
}

Matrix4f Rendering::Camera::GetProjectionMatrix(float aspectRatio) const
{
    return glm::perspective(glm::radians(fieldOfView), aspectRatio, nearPlane, farPlane);
}
