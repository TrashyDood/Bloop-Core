#define GLM_ENABLE_EXPERIMENTAL
#include "Transform.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/norm.hpp>

Matrix4f Transform::GetTransformMatrixScaled() const
{
    return glm::scale(glm::translate(Matrix4f{1.0f}, position) * rotationMatrix, scale);
}

Matrix4f Transform::GetTransformMatrixUnScaled() const
{
    return glm::translate(Matrix4f{1.0f}, position) * rotationMatrix;
}

Matrix4f Transform::GetTransformMatrix(const bool includeScale) const
{
    return includeScale
        ? GetTransformMatrixScaled()
        : GetTransformMatrixUnScaled();
}

Matrix4f Transform::GetLookMatrix() const
{
    return glm::lookAt(position, position + GetForward(), GetUp());
}


Vector3f Transform::TransformVector(const Vector3f &vector, const bool includeScale) const
{
    return Vector3f{GetTransformMatrix(includeScale) * Vector4f{vector, 0.0f}};
}

Vector3f Transform::TransformPoint(const Vector3f &point, const bool includeScale) const
{
    return Vector3f{GetTransformMatrix(includeScale) * Vector4f{point, 1.0f}};
}

void Transform::SetRotation(const Quaternionf &newRotation)
{
    rotation = glm::dot(newRotation, newRotation) > 0.0f ?
        glm::normalize(newRotation) : Quaternionf{1.0f, 0.0f, 0.0f, 0.0f};

    rotationEuler = glm::eulerAngles(rotation);
    rotationMatrix = glm::mat4_cast(rotation);
}

void Transform::SetLookDirection(const Vector3f &direction, const Vector3f &up)
{
    if (glm::length2(direction) <= SMALL_NUMBER || glm::length2(up) <= SMALL_NUMBER)
        return;

    const Vector3f normalizedDirection = glm::normalize(direction),
        normalizedUp = glm::normalize(up);

    if (glm::length2(glm::cross(normalizedDirection, normalizedUp)) <= SMALL_NUMBER)
        return;

    SetRotation(glm::quatLookAt(normalizedDirection, normalizedUp));
}