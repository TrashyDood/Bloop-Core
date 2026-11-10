#define GLM_ENABLE_EXPERIMENTAL
#include "Transform.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/norm.hpp>

#define EPSILON 1e-6f

Matrix4f Shared::Transform::GetTransformMatrixScaled() const
{
    return glm::scale(glm::translate(Matrix4f{1.0f}, position) * rotationMatrix, scale);
}

Matrix4f Shared::Transform::GetTransformMatrixUnScaled() const
{
    return glm::translate(Matrix4f{1.0f}, position) * rotationMatrix;
}

Matrix4f Shared::Transform::GetTransformMatrix(const bool includeScale) const
{
    return includeScale
        ? GetTransformMatrixScaled()
        : GetTransformMatrixUnScaled();
}

Matrix4f Shared::Transform::GetLookMatrix() const
{
    return glm::lookAt(position, position + GetForward(), GetUp());
}


Vector3f Shared::Transform::TransformVector(const Vector3f &vector, const bool includeScale) const
{
    return Vector3f{GetTransformMatrix(includeScale) * Vector4f{vector, 0.0f}};
}

Vector3f Shared::Transform::TransformPoint(const Vector3f &point, const bool includeScale) const
{
    return Vector3f{GetTransformMatrix(includeScale) * Vector4f{point, 1.0f}};
}

void Shared::Transform::SetRotation(const Quaternionf &newRotation)
{
    rotation = glm::dot(newRotation, newRotation) > 0.0f ?
        glm::normalize(newRotation) : Quaternionf{1.0f, 0.0f, 0.0f, 0.0f};

    rotationEuler = glm::eulerAngles(rotation);
    rotationMatrix = glm::mat4_cast(rotation);
}

void Shared::Transform::SetLookDirection(const Vector3f &direction, const Vector3f &up)
{
    if (glm::length2(direction) <= EPSILON || glm::length2(up) <= EPSILON)
        return;

    const Vector3f normalizedDirection = glm::normalize(direction),
        normalizedUp = glm::normalize(up);

    if (glm::length2(glm::cross(normalizedDirection, normalizedUp)) <= EPSILON)
        return;

    SetRotation(glm::quatLookAt(normalizedDirection, normalizedUp));
}