#include "GlfwInput.h"

namespace Graphics
{
    KeyCode ToSharedKeyCode(const int key) noexcept
    {
        if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9)
            return static_cast<KeyCode>(static_cast<int>(KeyCode::Num0) + key - GLFW_KEY_0);
        if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z)
            return static_cast<KeyCode>(static_cast<int>(KeyCode::A) + key - GLFW_KEY_A);
        if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F25)
            return static_cast<KeyCode>(static_cast<int>(KeyCode::F1) + key - GLFW_KEY_F1);
        if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_9)
            return static_cast<KeyCode>(
                static_cast<int>(KeyCode::Keypad0) + key - GLFW_KEY_KP_0);

        switch (key)
        {
        case GLFW_KEY_UNKNOWN: return KeyCode::Unknown;
        case GLFW_KEY_SPACE: return KeyCode::Space;
        case GLFW_KEY_APOSTROPHE: return KeyCode::Apostrophe;
        case GLFW_KEY_COMMA: return KeyCode::Comma;
        case GLFW_KEY_MINUS: return KeyCode::Minus;
        case GLFW_KEY_PERIOD: return KeyCode::Period;
        case GLFW_KEY_SLASH: return KeyCode::Slash;
        case GLFW_KEY_SEMICOLON: return KeyCode::Semicolon;
        case GLFW_KEY_EQUAL: return KeyCode::Equal;
        case GLFW_KEY_LEFT_BRACKET: return KeyCode::LeftBracket;
        case GLFW_KEY_BACKSLASH: return KeyCode::Backslash;
        case GLFW_KEY_RIGHT_BRACKET: return KeyCode::RightBracket;
        case GLFW_KEY_GRAVE_ACCENT: return KeyCode::GraveAccent;
        case GLFW_KEY_WORLD_1: return KeyCode::World1;
        case GLFW_KEY_WORLD_2: return KeyCode::World2;
        case GLFW_KEY_ESCAPE: return KeyCode::Escape;
        case GLFW_KEY_ENTER: return KeyCode::Enter;
        case GLFW_KEY_TAB: return KeyCode::Tab;
        case GLFW_KEY_BACKSPACE: return KeyCode::Backspace;
        case GLFW_KEY_INSERT: return KeyCode::Insert;
        case GLFW_KEY_DELETE: return KeyCode::Delete;
        case GLFW_KEY_RIGHT: return KeyCode::Right;
        case GLFW_KEY_LEFT: return KeyCode::Left;
        case GLFW_KEY_DOWN: return KeyCode::Down;
        case GLFW_KEY_UP: return KeyCode::Up;
        case GLFW_KEY_PAGE_UP: return KeyCode::PageUp;
        case GLFW_KEY_PAGE_DOWN: return KeyCode::PageDown;
        case GLFW_KEY_HOME: return KeyCode::Home;
        case GLFW_KEY_END: return KeyCode::End;
        case GLFW_KEY_CAPS_LOCK: return KeyCode::CapsLock;
        case GLFW_KEY_SCROLL_LOCK: return KeyCode::ScrollLock;
        case GLFW_KEY_NUM_LOCK: return KeyCode::NumLock;
        case GLFW_KEY_PRINT_SCREEN: return KeyCode::PrintScreen;
        case GLFW_KEY_PAUSE: return KeyCode::Pause;
        case GLFW_KEY_KP_DECIMAL: return KeyCode::KeypadDecimal;
        case GLFW_KEY_KP_DIVIDE: return KeyCode::KeypadDivide;
        case GLFW_KEY_KP_MULTIPLY: return KeyCode::KeypadMultiply;
        case GLFW_KEY_KP_SUBTRACT: return KeyCode::KeypadSubtract;
        case GLFW_KEY_KP_ADD: return KeyCode::KeypadAdd;
        case GLFW_KEY_KP_ENTER: return KeyCode::KeypadEnter;
        case GLFW_KEY_KP_EQUAL: return KeyCode::KeypadEqual;
        case GLFW_KEY_LEFT_SHIFT: return KeyCode::LeftShift;
        case GLFW_KEY_LEFT_CONTROL: return KeyCode::LeftControl;
        case GLFW_KEY_LEFT_ALT: return KeyCode::LeftAlt;
        case GLFW_KEY_LEFT_SUPER: return KeyCode::LeftSuper;
        case GLFW_KEY_RIGHT_SHIFT: return KeyCode::RightShift;
        case GLFW_KEY_RIGHT_CONTROL: return KeyCode::RightControl;
        case GLFW_KEY_RIGHT_ALT: return KeyCode::RightAlt;
        case GLFW_KEY_RIGHT_SUPER: return KeyCode::RightSuper;
        case GLFW_KEY_MENU: return KeyCode::Menu;
        default: return KeyCode::Unknown;
        }
    }

    MouseKeyCode ToSharedMouseButton(const int button) noexcept
    {
        if (button >= GLFW_MOUSE_BUTTON_1 && button <= GLFW_MOUSE_BUTTON_8)
            return static_cast<MouseKeyCode>(
                static_cast<int>(MouseKeyCode::Button1) + button - GLFW_MOUSE_BUTTON_1);

        return MouseKeyCode::Unknown;
    }

    std::optional<KeyAction> ToSharedKeyAction(const int action) noexcept
    {
        switch (action)
        {
        case GLFW_PRESS: return Pressed;
        case GLFW_RELEASE: return Released;
        case GLFW_REPEAT: return Held;
        default: return std::nullopt;
        }
    }
}
