#pragma once

#include "Shared/Shared.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Shared
{
    class Transform
    {
        Vector3f position{0.0f};
        Vector3f eulerRotation{0.0f};
        Vector3f scale{1.0f};

    public:
        [[nodiscard]] Matrix4f GetRotationMatrix() const;
        [[nodiscard]] Matrix4f GetModelMatrix() const;

        [[nodiscard]] const Vector3f &GetPosition() const { return position; }
        [[nodiscard]] const Vector3f &GetEulerRotation() const { return eulerRotation; }
        [[nodiscard]] const Vector3f &GetScale() const { return scale; }

        void SetPosition(const Vector3f &newPosition) { position = newPosition; }
        void SetRotation(const Vector3f &newRotation) { eulerRotation = newRotation; }
        void SetScale(const Vector3f &newScale) { scale = newScale; }

        void Translate(const Vector3f &offset) { position += offset; }
        void Rotate(const Vector3f &angles) { eulerRotation += angles; }
    };
}
