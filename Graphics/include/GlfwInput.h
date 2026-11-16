#pragma once

#include "InputCodes.h"
#include "Shared.h"

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif

#include <GLFW/glfw3.h>

#include <optional>

namespace Graphics
{
    Input::KeyCode ToSharedKeyCode(Int32 glfwKey) noexcept;
    Input::MouseKeyCode ToSharedMouseButton(Int32 glfwButton) noexcept;
    std::optional<Input::KeyAction> ToSharedKeyAction(Int32 glfwAction) noexcept;
}
