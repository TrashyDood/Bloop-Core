#include "Shared/Transform.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

void Shared::Transform::SetRotation(const Quaternionf &newRotation)
{
    const float magnitudeSquared = glm::dot(newRotation, newRotation);
    rotation = magnitudeSquared > 0.0f ? glm::normalize(newRotation) : Quaternionf{1.0f, 0.0f, 0.0f, 0.0f};
    rotationEuler = glm::eulerAngles(rotation);
    rotationMatrix = glm::mat4_cast(rotation);
}

bool Shared::Transform::SetLookAt(const Vector3f &target, const Vector3f &up)
{
    const Vector3f direction = target - position;
    if (glm::dot(direction, direction) <= 1e-6f || glm::dot(up, up) <= 1e-6f)
        return false;

    const Vector3f normalizedDirection = glm::normalize(direction);
    const Vector3f normalizedUp = glm::normalize(up);
    const Vector3f right = glm::cross(normalizedDirection, normalizedUp);
    if (glm::dot(right, right) <= 1e-6f)
        return false;

    SetRotation(glm::quatLookAtRH(normalizedDirection, normalizedUp));
    return true;
}

Matrix4f Shared::Transform::GetModelMatrix() const
{
    return glm::scale(glm::translate(Matrix4f{1.0f}, position) * rotationMatrix, scale);
}

Vector3f Shared::Transform::TransformVector(const Vector3f &vector, bool includeScale) const
{
    const Matrix4f transformMatrix = includeScale
        ? GetModelMatrix()
        : glm::translate(Matrix4f{1.0f}, position) * rotationMatrix;
    return Vector3f{transformMatrix * Vector4f{vector, 0.0f}};
}

Vector3f Shared::Transform::TransformPosition(const Vector3f &point, bool includeScale) const
{
    const Matrix4f transformMatrix = includeScale
        ? GetModelMatrix()
        : glm::translate(Matrix4f{1.0f}, position) * rotationMatrix;
    return Vector3f{transformMatrix * Vector4f{point, 1.0f}};
}

void Shared::Transform::TranslateLocal(const Vector3f &offset)
{
    position += GetRight() * offset.x + GetUp() * offset.y + GetForward() * offset.z;
}