#define GLM_ENABLE_EXPERIMENTAL
#include "Transform.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/norm.hpp>

#pragma region Transform class
Matrix4f Engine::Transform::GetTransformMatrix(const bool includeScale) const
{
    return includeScale
        ? Engine::GetTransformMatrix(GetPosition(), GetScale(),  GetRotationMatrix())
        : Engine::GetTransformMatrix(GetPosition(), GetRotationMatrix());
}

Matrix4f Engine::Transform::GetLookMatrix() const
{
    return Engine::GetLookMatrix(GetPosition(), GetForward(), GetUp());
}

Vector3f Engine::Transform::TransformVector(const Vector3f &vector, const bool includeScale) const
{
    return Engine::TransformVector(GetTransformMatrix(includeScale), vector);
}

Vector3f Engine::Transform::TransformPoint(const Vector3f &point, const bool includeScale) const
{
    return Engine::TransformPoint(GetTransformMatrix(includeScale), point);
}

void Engine::Transform::SetRotation(const Quaternionf &newRotation)
{
    rotation = glm::dot(newRotation, newRotation) > 0.0f ?
        glm::normalize(newRotation) : Quaternionf{1.0f, 0.0f, 0.0f, 0.0f};

    rotationEuler = glm::eulerAngles(rotation);
    rotationMatrix = glm::mat4_cast(rotation);
}

void Engine::Transform::SetLookDirection(const Vector3f &direction, const Vector3f &up)
{
    if (glm::length2(direction) <= SMALL_NUMBER || glm::length2(up) <= SMALL_NUMBER)
        return;

    const Vector3f normalizedDirection = glm::normalize(direction),
        normalizedUp = glm::normalize(up);

    if (glm::length2(glm::cross(normalizedDirection, normalizedUp)) <= SMALL_NUMBER)
        return;

    SetRotation(glm::quatLookAt(normalizedDirection, normalizedUp));
}

void Engine::Transform::SetLookAtPoint(const Vector3f &target, const Vector3f &up)
{
    SetLookDirection(target - position, up);
}
#pragma endregion

Matrix4f Engine::GetTransformMatrix(const Vector3f &position,
    const Vector3f &scale, const Matrix4f &rotationMatrix)
{
    return glm::scale(glm::translate(Matrix4f{1.0f}, position) * rotationMatrix, scale);
}

Matrix4f Engine::GetTransformMatrix(const Vector3f &position,
    const Matrix4f &rotationMatrix)
{
    return glm::translate(Matrix4f{1.0f}, position) * rotationMatrix;
}

Matrix4f Engine::GetLookMatrix(const Vector3f &position,
    const Vector3f &forward, const Vector3f &up)
{
    return glm::lookAt(position, position + forward, up);
}

Vector3f Engine::TransformVector(const Matrix4f &transformMatrix, const Vector3f &vector)
{
    return Vector3f{transformMatrix * Vector4f{vector, 0.f}};
}

Vector3f Engine::TransformPoint(const Matrix4f &transformMatrix, const Vector3f &point)
{
    return Vector3f{transformMatrix * Vector4f{point, 1.f}};
}
