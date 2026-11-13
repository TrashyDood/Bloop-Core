#pragma once

#include "InputCodes.h"

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif

#include <GLFW/glfw3.h>

#include <optional>

namespace Graphics
{
    KeyCode ToSharedKeyCode(int glfwKey) noexcept;
    MouseKeyCode ToSharedMouseButton(int glfwButton) noexcept;
    std::optional<KeyAction> ToSharedKeyAction(int glfwAction) noexcept;
}
