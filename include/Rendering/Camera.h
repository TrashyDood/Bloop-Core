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
        static constexpr Vector3f DefaultResetPosition{0.f, 0.f, 3.f},
            DefaultLookAt = {0.f, 0.f, 0.f};
        static constexpr float MovementSpeed = 5.f,
            MinimumOrbitDistance = 0.1f,
            OrbitFocusDistance = 3.f;

        Shared::Transform transform;
        Vector3f orbitTarget{0.f};
        bool hasOrbitTarget = false;

        [[nodiscard]] Vector3f &GetOrbitTarget()
        {
            if (!hasOrbitTarget)
            {
                orbitTarget = GetTransform().GetPosition() + GetTransform().GetForward() * OrbitFocusDistance;
                hasOrbitTarget = true;
            }
            return orbitTarget;
        }

    public:
        FlyingCamera(const float fieldOfView = 45.f, const float nearPlane = 0.1f, const float farPlane = 100.f)
            : Camera(transform, fieldOfView, nearPlane, farPlane) {}

        void ResetPosition(Vector3f resetPosition = DefaultResetPosition, Vector3f lookAt = DefaultLookAt)
        {
            GetTransform().SetPosition(resetPosition);
            GetTransform().SetLookAt(lookAt, Vectors::v3fUp);
            hasOrbitTarget = false;
        }

        void Move(const Vector3f &direction, float deltaTime)
        {
            const float length = glm::length(direction);
            const Vector3f normalizedDirection = length > 1.0f ? direction / length : direction;
            const float distance = MovementSpeed * deltaTime;
            const Vector3f oldPosition = GetTransform().GetPosition();
            GetTransform().TranslateLocal({normalizedDirection.x * distance, 0.0f, normalizedDirection.z * distance});
            GetTransform().Translate(Vectors::v3fUp * normalizedDirection.y * MovementSpeed * deltaTime);
            if (hasOrbitTarget)
                orbitTarget += GetTransform().GetPosition() - oldPosition;
        }

        bool Orbit(float yawDeltaRadians, float pitchDeltaRadians)
        {
            const Vector3f target = GetOrbitTarget();
            const Vector3f offset = GetTransform().GetPosition() - target;
            const float distance = glm::length(offset);
            if (distance <= 1e-6f)
                return false;

            const float pitch = glm::asin(glm::clamp(
                glm::dot(offset, Vectors::v3fUp) / distance, -1.0f, 1.0f));
            const float clampedPitchDelta =
                glm::clamp(pitch + pitchDeltaRadians,
                           glm::radians(-89.0f), glm::radians(89.0f)) -
                pitch;
            const Quaternionf yawRotation = glm::angleAxis(yawDeltaRadians, Vectors::v3fUp);
            const Vector3f yawedOffset = yawRotation * offset;
            const Vector3f pitchAxis = yawRotation * Vectors::v3fRight;
            const Quaternionf pitchRotation = glm::angleAxis(clampedPitchDelta, pitchAxis);

            Shared::Transform orbitTransform = GetTransform();
            orbitTransform.SetPosition(target + pitchRotation * yawedOffset);
            if (!orbitTransform.SetLookAt(target, Vectors::v3fUp))
                return false;

            GetTransform().SetPosition(orbitTransform.GetPosition());
            GetTransform().SetRotation(orbitTransform.GetRotation());
            return true;
        }

        bool Zoom(float distanceDelta)
        {
            const Vector3f target = GetOrbitTarget();
            const Vector3f offset = GetTransform().GetPosition() - target;
            const float distance = glm::length(offset);
            if (distance <= 1e-6f)
                return false;

            const float newDistance = glm::max(
                distance -(distanceDelta * glm::distance(GetTransform().GetPosition(), target)),
                MinimumOrbitDistance);

            GetTransform().SetPosition(target + offset * (newDistance / distance));
            return true;
        }
    };
}