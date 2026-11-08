#include "Shared/Transform.h"
#include <glm/gtc/matrix_transform.hpp>

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
    const float directionLengthSquared = glm::dot(direction, direction);
    const float upLengthSquared = glm::dot(up, up);
    if (directionLengthSquared <= 1e-6f || upLengthSquared <= 1e-6f)
        return false;

    const Vector3f rightUnnormalized = glm::cross(direction, up);
    const float rightLengthSquared = glm::dot(rightUnnormalized, rightUnnormalized);
    if (rightLengthSquared <= 1e-6f * directionLengthSquared * upLengthSquared)
        return false;

    const Vector3f forward = direction / glm::sqrt(directionLengthSquared);
    const Vector3f right = rightUnnormalized / glm::sqrt(rightLengthSquared);
    const Vector3f correctedUp = glm::cross(right, forward);

    Matrix3f basis{1.0f};
    basis[0] = right;
    basis[1] = correctedUp;
    basis[2] = -forward;
    SetRotation(glm::quat_cast(basis));
    return true;
}

Matrix4f Shared::Transform::GetModelMatrix() const
{
    return glm::scale(glm::translate(Matrix4f{1.0f}, position) * rotationMatrix, scale);
}

Vector3f Shared::Transform::GetRight() const
{
    return glm::normalize(Vector3f{rotationMatrix * Vectors::v4fRight});
}

Vector3f Shared::Transform::GetUp() const
{
    return glm::normalize(Vector3f{rotationMatrix * Vectors::v4fUp});
}

Vector3f Shared::Transform::GetForward() const
{
    return glm::normalize(Vector3f{rotationMatrix * Vectors::v4fForward});
}

void Shared::Transform::TranslateLocal(const Vector3f &offset)
{
    position += GetRight() * offset.x + GetUp() * offset.y + GetForward() * offset.z;
}

void Shared::Transform::RotateAxis(float angleRadians, const Vector3f &axis)
{
    const float axisLengthSquared = glm::dot(axis, axis);
    if (axisLengthSquared <= 0.0f)
        return;

    Rotate(glm::angleAxis(angleRadians, axis / glm::sqrt(axisLengthSquared)));
}