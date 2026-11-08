#pragma once

#include "Rendering/Rendering.h"
#include "Shared/Transform.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Rendering
{
    class Camera
    {
        Shared::Transform transform;
        float fieldOfView = 45.0f;
        float nearPlane = 0.1f;
        float farPlane = 100.0f;

        [[nodiscard]] Vector3f GetDirection(const Vector3f &localDirection) const;

    public:
        [[nodiscard]] float GetFieldOfView() const { return fieldOfView; }
        [[nodiscard]] float GetNearPlane() const { return nearPlane; }
        [[nodiscard]] float GetFarPlane() const { return farPlane; }

        [[nodiscard]] Shared::Transform &GetTransform() { return transform; }
        [[nodiscard]] const Shared::Transform &GetTransform() const { return transform; }

        [[nodiscard]] Matrix4f GetViewMatrix() const;
        [[nodiscard]] Matrix4f GetProjectionMatrix(float aspectRatio = DEFAULT_ASPECT_RATIO) const;

        Camera() { transform.SetPosition({0.0f, 0.0f, 3.0f}); }

        void Rotate(float yawOffset, float pitchOffset)
        {
            Vector3f eulerRotation = transform.GetEulerRotation();
            eulerRotation.y -= glm::radians(yawOffset);
            eulerRotation.x = glm::clamp(
                eulerRotation.x + glm::radians(pitchOffset),
                glm::radians(-89.0f),
                glm::radians(89.0f));
            transform.SetRotation(eulerRotation);
        }

        void Zoom(float amount)
        {
            transform.Translate(GetDirection({0.0f, 0.0f, -1.0f}) * amount);
        }

        void Move(float forwardDistance, float rightDistance)
        {
            const Vector3f front = GetDirection({0.0f, 0.0f, -1.0f});
            const Vector3f right = GetDirection({1.0f, 0.0f, 0.0f});
            transform.Translate(front * forwardDistance + right * rightDistance);
        }
    };
}