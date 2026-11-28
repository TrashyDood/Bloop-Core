#include "Shared.h"

namespace Matrices
{
    Matrix4f GetMatrix_Transform(const Vector3f &position, const Vector3f &scale, const Matrix4f &rotationMatrix)
    {
        return glm::scale(glm::translate(Matrix4f{1.0f}, position) * rotationMatrix, scale);
    }

    Matrix4f GetMatrix_Transform(const Vector3f &position, const Matrix4f &rotationMatrix)
    {
        return glm::translate(Matrix4f{1.0f}, position) * rotationMatrix;
    }

    Matrix4f GetMatrix_Look(const Vector3f &position, const Vector3f &forward, const Vector3f &up)
    {
        return glm::lookAt(position, position + forward, up);
    }

    Matrix4f GetMatrix_Rotation(const Quaternion &rotation)
    {
        return glm::mat4_cast(rotation);
    }

    Vector3f TransformVector(const Matrix4f &transformMatrix, const Vector3f &vector)
    {
        return Vector3f{transformMatrix * Vector4f{vector, 0.f}};
    }

    Vector3f TransformPoint(const Matrix4f &transformMatrix, const Vector3f &point)
    {
        return Vector3f{transformMatrix * Vector4f{point, 1.f}};
    }

    Vector3f GetPosition(const Matrix4f &transformMatrix)
    {
        return Vector3f{transformMatrix[3]};
    }

    Vector3f GetScale(const Matrix4f &transformMatrix)
    {
        return Vector3f{glm::length(Vector3f{transformMatrix[0]}), glm::length(Vector3f{transformMatrix[1]}),
            glm::length(Vector3f{transformMatrix[2]})};
    }

    Quaternion GetRotation(const Matrix4f &transformMatrix)
    {
        glm::mat3 rotationMatrix{transformMatrix};

        for (int axisIndex = 0; axisIndex < 3; ++axisIndex)
        {
            rotationMatrix[axisIndex] /= glm::length(rotationMatrix[axisIndex]);
        }

        return glm::quat_cast(rotationMatrix);
    }

    // Euler angles in radians
    Vector3f GetEulerRotation(const Matrix4f &transformMatrix)
    {
        return glm::eulerAngles(GetRotation(transformMatrix));
    }
} // namespace Matrices