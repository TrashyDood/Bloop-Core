#pragma once

#include "Rendering/Rendering.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Rendering
{
    class Camera
    {
        Vector3f target{0.0f, 0.0f, 0.0f};
        Vector3f worldUp{0.0f, 1.0f, 0.0f};
        Vector3f position{0.0f, 0.0f, 3.0f};
        float yaw = -90.0f;
        float pitch = 0.0f;
        float distance = 3.0f;
        float fieldOfView = 45.0f;
        float nearPlane = 0.1f;
        float farPlane = 100.0f;

        void UpdateCameraPosition()
        {
            const float yawRadians = glm::radians(yaw);
            const float pitchRadians = glm::radians(pitch);

            const Vector3f direction{
                glm::cos(yawRadians) * glm::cos(pitchRadians),
                glm::sin(pitchRadians),
                glm::sin(yawRadians) * glm::cos(pitchRadians),
            };

            position = target + direction * distance;
        }

    public:
        Camera() { UpdateCameraPosition(); }

        [[nodiscard]] Matrix4f GetViewMatrix() const
        {
            const Vector3f front = glm::normalize(target - position);
            Vector3f right = glm::normalize(glm::cross(front, worldUp));
            if (glm::dot(right, right) < 1e-6f)
            {
                right = glm::normalize(glm::cross(front, Vector3f{1.0f, 0.0f, 0.0f}));
            }

            const Vector3f upDirection = glm::cross(right, front);
            return glm::lookAt(position, target, upDirection);
        }

        [[nodiscard]] Matrix4f GetProjectionMatrix(float aspectRatio = DEFAULT_ASPECT_RATIO) const
        {
            return glm::perspective(glm::radians(fieldOfView), aspectRatio, nearPlane, farPlane);
        }

        void Rotate(float yawOffset, float pitchOffset)
        {
            yaw += yawOffset;
            pitch = glm::clamp(pitch + pitchOffset, -89.0f, 89.0f);
            UpdateCameraPosition();
        }

        void Zoom(float amount)
        {
            SetDistance(distance - amount);
        }

        void Move(float forwardDistance, float rightDistance)
        {
            const Vector3f front = glm::normalize(target - position);
            const Vector3f right = glm::normalize(glm::cross(front, worldUp));
            const Vector3f movement = front * forwardDistance + right * rightDistance;
            target += movement;
            UpdateCameraPosition();
        }

        void SetTarget(const Vector3f &newTarget)
        {
            target = newTarget;
            UpdateCameraPosition();
        }

        void SetDistance(float newDistance)
        {
            distance = glm::max(newDistance, 0.1f);
            UpdateCameraPosition();
        }
    };
}