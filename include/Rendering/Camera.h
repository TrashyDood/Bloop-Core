#pragma once

#include "Shared/Core.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Rendering
{
    class Camera
    {
        Vector3f target{0.0f, 0.0f, 0.0f};
        Vector3f up{0.0f, 1.0f, 0.0f};
        float yaw = -90.0f;
        float pitch = 0.0f;
        float mouseSensitivity = 0.1f;
        float distance = 3.0f;
        float fieldOfView = 45.0f;
        float nearPlane = 0.1f;
        float farPlane = 100.0f;

    public:
        [[nodiscard]] Matrix4f GetViewMatrix() const
        {
            const float yawRadians = glm::radians(yaw);
            const float pitchRadians = glm::radians(pitch);
            const Vector3f direction{
                glm::cos(yawRadians) * glm::cos(pitchRadians),
                glm::sin(pitchRadians),
                glm::sin(yawRadians) * glm::cos(pitchRadians),
            };
            return glm::lookAt(target - direction * distance, target, up);
        }

        [[nodiscard]] Matrix4f GetProjectionMatrix(float aspectRatio) const
        {
            return glm::perspective(glm::radians(fieldOfView), aspectRatio, nearPlane, farPlane);
        }

        void ProcessMouseMovement(float xOffset, float yOffset)
        {
            yaw += xOffset * mouseSensitivity;
            pitch = glm::clamp(pitch + yOffset * mouseSensitivity, -89.0f, 89.0f);
        }
    };
}