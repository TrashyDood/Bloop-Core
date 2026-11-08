#pragma once

#include "Rendering/Rendering.h"
#include "Shared/Transform.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Rendering
{
    class Camera
    {
        Shared::Transform &transform;
        float fieldOfView, nearPlane, farPlane;

    public:
        Camera(Shared::Transform &transform, const float fieldOfView = 45.f,
               const float nearPlane = 0.1f, const float farPlane = 100.f)
            : transform(transform), fieldOfView(fieldOfView),
              nearPlane(nearPlane), farPlane(farPlane) {}

        [[nodiscard]] float GetFieldOfView() const { return fieldOfView; }
        [[nodiscard]] float GetNearPlane() const { return nearPlane; }
        [[nodiscard]] float GetFarPlane() const { return farPlane; }

        [[nodiscard]] Shared::Transform &GetTransform() { return transform; }
        [[nodiscard]] const Shared::Transform &GetTransform() const { return transform; }

        [[nodiscard]] Matrix4f GetViewMatrix() const;
        [[nodiscard]] Matrix4f GetProjectionMatrix(float aspectRatio = DEFAULT_ASPECT_RATIO) const;

        void SetFieldOfView(const float newFieldOfView) { fieldOfView = newFieldOfView; }
        void SetNearPlane(const float newNearPlane) { nearPlane = newNearPlane; }
        void SetFarPlane(const float newFarPlane) { farPlane = newFarPlane; }
    };

    class FlyingCamera : public Camera
    {
        Shared::Transform transform;
        static constexpr float MovementSpeed = 5.f;
        static constexpr float MinimumOrbitDistance = 0.1f;

    public:
        FlyingCamera(const float fieldOfView = 45.f, const float nearPlane = 0.1f, const float farPlane = 100.f)
            : Camera(transform, fieldOfView, nearPlane, farPlane) {}

        void Move(const Vector3f &direction, float deltaTime)
        {
            const float length = glm::length(direction);
            const Vector3f normalizedDirection = length > 1.0f ? direction / length : direction;
            const float distance = MovementSpeed * deltaTime;
            GetTransform().TranslateLocal({normalizedDirection.x * distance, 0.0f, normalizedDirection.z * distance});
            GetTransform().Translate(Vectors::v3fUp * normalizedDirection.y * MovementSpeed * deltaTime);
        }

        bool Orbit(const Vector3f &target, float yawDeltaRadians, float pitchDeltaRadians)
        {
            const Vector3f offset = GetTransform().GetPosition() - target;
            const float distance = glm::length(offset);
            if (distance <= 1e-6f)
                return false;

            const float yaw = glm::atan(offset.x, offset.z) + yawDeltaRadians;
            const float pitch = glm::clamp(
                glm::asin(glm::clamp(offset.y / distance, -1.0f, 1.0f)) + pitchDeltaRadians,
                glm::radians(-89.0f),
                glm::radians(89.0f));
            const float horizontalDistance = distance * glm::cos(pitch);

            Shared::Transform orbitTransform = GetTransform();
            orbitTransform.SetPosition(target + Vector3f{
                horizontalDistance * glm::sin(yaw),
                distance * glm::sin(pitch),
                horizontalDistance * glm::cos(yaw),
            });
            if (!orbitTransform.SetLookAt(target))
                return false;

            GetTransform().SetPosition(orbitTransform.GetPosition());
            GetTransform().SetRotation(orbitTransform.GetRotation());
            return true;
        }

        bool Zoom(const Vector3f &target, float distanceDelta)
        {
            const Vector3f offset = GetTransform().GetPosition() - target;
            const float distance = glm::length(offset);
            if (distance <= 1e-6f)
                return false;

            const float newDistance =
            glm::max(
                distance -(distanceDelta * glm::distance(GetTransform().GetPosition(), target)),
                MinimumOrbitDistance);

            GetTransform().SetPosition(target + offset * (newDistance / distance));
            return true;
        }
    };
}