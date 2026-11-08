#include "Shared/Transform.h"

Matrix4f Shared::Transform::GetRotationMatrix() const
{
    Matrix4f result{1.0f};
    result = glm::rotate(result, eulerRotation.x, Vector3f{1.0f, 0.0f, 0.0f});
    result = glm::rotate(result, eulerRotation.y, Vector3f{0.0f, 1.0f, 0.0f});
    result = glm::rotate(result, eulerRotation.z, Vector3f{0.0f, 0.0f, 1.0f});
    return result;
}

Matrix4f Shared::Transform::GetModelMatrix() const
{
    return glm::scale(glm::translate(Matrix4f{1.0f}, position) * GetRotationMatrix(), scale);
}
