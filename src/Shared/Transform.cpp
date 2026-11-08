#define GLM_ENABLE_EXPERIMENTAL
#include "Shared/Transform.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/norm.hpp>

#define EPSILON 1e-6f

Matrix4f Shared::Transform::GetTransformMatrix(const bool includeScale) const
{
    return includeScale
        ? glm::scale(glm::translate(Matrix4f{1.0f}, position) * rotationMatrix, scale)
        : glm::translate(Matrix4f{1.0f}, position) * rotationMatrix;
}

void Shared::Transform::SetRotation(const Quaternionf &newRotation)
{
    const float magnitudeSquared = glm::dot(newRotation, newRotation);
    rotation = magnitudeSquared > 0.0f ? glm::normalize(newRotation) : Quaternionf{1.0f, 0.0f, 0.0f, 0.0f};
    rotationEuler = glm::eulerAngles(rotation);
    rotationMatrix = glm::mat4_cast(rotation);
}

void Shared::Transform::SetLookAt(const Vector3f &target, const Vector3f &up)
{
    const Vector3f direction = target - position;

    if (glm::length2(direction) <= EPSILON || glm::length2(up) <= EPSILON)
        return;

    const Vector3f normalizedDirection = glm::normalize(direction);
    const Vector3f normalizedUp = glm::normalize(up);
    const Vector3f right = glm::cross(normalizedDirection, normalizedUp);

    if (glm::length2(right) <= EPSILON) return;

    SetRotation(glm::quatLookAtRH(normalizedDirection, normalizedUp));
}

Vector3f Shared::Transform::TransformVector(const Vector3f &vector, const bool includeScale) const
{
    return Vector3f{GetTransformMatrix(includeScale) * Vector4f{vector, 0.0f}};
}

Vector3f Shared::Transform::TransformPosition(const Vector3f &point, const bool includeScale) const
{
    return Vector3f{GetTransformMatrix(includeScale) * Vector4f{point, 1.0f}};
}