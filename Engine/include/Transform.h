#pragma once

#include "Shared.h"

namespace Engine
{
    class Transform
    {
        Vector3f position, scale;
        Quaternion rotation;

    public:
        [[nodiscard]] const Vector3f &GetPosition() const { return position; }
        [[nodiscard]] const Vector3f &GetScale() const { return scale; }
        [[nodiscard]] const Quaternion &GetRotation() const { return rotation; }
        [[nodiscard]] const Vector3f GetRotationEuler() const { return glm::eulerAngles(rotation); }

        [[nodiscard]] Vector3f GetRight() const { return TransformVector(Vectors::v3fRight, false); }
        [[nodiscard]] Vector3f GetUp() const { return TransformVector(Vectors::v3fUp, false); }
        [[nodiscard]] Vector3f GetForward() const { return TransformVector(Vectors::v3fForward, false); }

        [[nodiscard]] Matrix4f GetTransformMatrix(const bool includeScale = true) const;
        [[nodiscard]] const Matrix4f GetRotationMatrix() const;
        [[nodiscard]] Matrix4f GetLookMatrix() const;

        [[nodiscard]] Vector3f TransformVector(const Vector3f &vector, const bool includeScale = true) const;
        [[nodiscard]] Vector3f TransformPoint(const Vector3f &point, const bool includeScale = true) const;

        Transform(const Vector3f &position = Vectors::v3fZero, const Quaternion &rotation = Quaternions::qfIdentity,
            const Vector3f &scale = Vectors::v3fOne)
        {
            SetPosition(position);
            SetRotation(rotation);
            SetScale(scale);
        }

        void SetPosition(const Vector3f &newPosition) { position = newPosition; }
        void SetScale(const Vector3f &newScale) { scale = newScale; }
        void SetRotation(const Quaternion &newRotation);

        void SetRotationEuler(const Vector3f &newRotationEuler);
        void SetLookDirection(const Vector3f &direction, const Vector3f &up = Vectors::v3fUp);
        void SetLookAtPoint(const Vector3f &target, const Vector3f &up = Vectors::v3fUp);

        void Translate(const Vector3f &offset) { SetPosition(position + offset); }
        void TranslateLocal(const Vector3f &offset) { Translate(TransformVector(offset, false)); }
        void ScaleBy(const Vector3f &factor) { SetScale(scale * factor); }
        void ScaleBy(float factor) { SetScale(scale * factor); }
        void Rotate(const Quaternion &rotationOffset) { SetRotation(rotation * rotationOffset); }
        void RotateEuler(const Vector3f &angles) { Rotate(Quaternion{angles}); }
    };

    [[nodiscard]]
    Matrix4f GetMatrix_Transform(const Vector3f &position, const Vector3f &scale, const Matrix4f &rotationMatrix);
    [[nodiscard]]
    Matrix4f GetMatrix_Transform(const Vector3f &position, const Matrix4f &rotationMatrix);
    [[nodiscard]]
    Matrix4f GetMatrix_Look(const Vector3f &position, const Vector3f &forward, const Vector3f &up);
    [[nodiscard]]
    Matrix4f GetMatrix_Rotation(const Quaternion &rotation);

    [[nodiscard]]
    Vector3f TransformVector(const Matrix4f &transformMatrix, const Vector3f &vector);
    [[nodiscard]]
    Vector3f TransformPoint(const Matrix4f &transformMatrix, const Vector3f &point);
}
