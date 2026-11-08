#pragma once

#include "Rendering/Rendering.h"
#include "Shared/Transform.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Rendering
{
    class Camera
    {
        Shared::Transform transform;
        float fieldOfView = 45.0f, nearPlane = 0.1f, farPlane = 100.0f;

    public:
        Camera() { transform.SetPosition({0.0f, 0.0f, 3.0f}); }

        [[nodiscard]] float GetFieldOfView() const { return fieldOfView; }
        [[nodiscard]] float GetNearPlane() const { return nearPlane; }
        [[nodiscard]] float GetFarPlane() const { return farPlane; }

        [[nodiscard]] Shared::Transform &GetTransform() { return transform; }
        [[nodiscard]] const Shared::Transform &GetTransform() const { return transform; }

        [[nodiscard]] Matrix4f GetViewMatrix() const;
        [[nodiscard]] Matrix4f GetProjectionMatrix(float aspectRatio = DEFAULT_ASPECT_RATIO) const;
    };
}