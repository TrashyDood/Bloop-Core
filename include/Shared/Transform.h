#pragma once

#include "Shared/Shared.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Shared
{
    class Transform
    {
        Vector3f position{0.0f};
        Vector3f rotation{0.0f};
        Vector3f scale{1.0f};

    public:
        [[nodiscard]] const Vector3f &GetPosition() const { return position; }
        [[nodiscard]] const Vector3f &GetRotation() const { return rotation; }
        [[nodiscard]] const Vector3f &GetScale() const { return scale; }

        void SetPosition(const Vector3f &newPosition) { position = newPosition; }
        void SetRotation(const Vector3f &newRotation) { rotation = newRotation; }
        void SetScale(const Vector3f &newScale) { scale = newScale; }

        void Translate(const Vector3f &offset) { position += offset; }
        void Rotate(const Vector3f &angles) { rotation += angles; }

        [[nodiscard]] Matrix4f GetModelMatrix() const
        {
            Matrix4f model{1.0f};
            model = glm::translate(model, position);
            model = glm::rotate(model, rotation.x, Vector3f{1.0f, 0.0f, 0.0f});
            model = glm::rotate(model, rotation.y, Vector3f{0.0f, 1.0f, 0.0f});
            model = glm::rotate(model, rotation.z, Vector3f{0.0f, 0.0f, 1.0f});
            return glm::scale(model, scale);
        }
    };
}
