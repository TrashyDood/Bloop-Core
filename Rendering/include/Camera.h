#pragma once

#include "Rendering.h"
#include "Transform.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Rendering
{
    class Camera
    {
        Shared::Transform &transform;
        bool isOrthographic = false;
        float fieldOfView = 90.f, nearPlane = 0.1f, farPlane = 100.f, orthographicSize = 5.f;

    public:
        Camera(Shared::Transform &transform)
            : transform(transform) {}

        [[nodiscard]] float GetFieldOfView() const { return fieldOfView; }
        [[nodiscard]] float GetNearPlane() const { return nearPlane; }
        [[nodiscard]] float GetFarPlane() const { return farPlane; }
        [[nodiscard]] bool IsOrthographic() const { return isOrthographic; }
        [[nodiscard]] float GetOrthographicSize() const { return orthographicSize; }

        [[nodiscard]] Shared::Transform &GetTransform() { return transform; }
        [[nodiscard]] const Shared::Transform &GetTransform() const { return transform; }

        [[nodiscard]] Matrix4f GetViewMatrix() const { return transform.GetLookMatrix(); }
        [[nodiscard]] Matrix4f GetProjectionMatrix(const float aspectRatio = DEFAULT_ASPECT_RATIO) const;
        [[nodiscard]] Matrix4f GetOrthographicMatrix(const float aspectRatio) const;
        [[nodiscard]] Matrix4f GetPerspectiveMatrix(const float aspectRatio) const;

        void SetFieldOfView(const float newFieldOfView) { fieldOfView = newFieldOfView; }
        void SetNearPlane(const float newNearPlane) { nearPlane = newNearPlane; }
        void SetFarPlane(const float newFarPlane) { farPlane = newFarPlane; }
        void SetIsOrthographic(const bool newIsOrthographic) { isOrthographic = newIsOrthographic; }
        void SetOrthographicSize(const float newOrthographicSize) { orthographicSize = newOrthographicSize; }
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
            : Camera(transform) {}

        void ResetPosition(Vector3f resetPosition = DefaultResetPosition, Vector3f lookAt = DefaultLookAt)
        {
            GetTransform().SetPosition(resetPosition);
            GetTransform().SetLookAtPoint(lookAt, Vectors::v3fUp);
            hasOrbitTarget = false;
        }

        void Move(const Vector3f &direction, float deltaTime)
        {
            const float length = glm::length(direction),
                distance = MovementSpeed * deltaTime;

            const Vector3f normalizedDirection = length > 1.0f ? direction / length : direction,
                oldPosition = GetTransform().GetPosition();

            GetTransform().Translate(
                GetTransform().GetRight() * normalizedDirection.x * distance +
                GetTransform().GetForward() * normalizedDirection.z * distance);
                
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
            orbitTransform.SetLookAtPoint(target, Vectors::v3fUp);

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