#pragma once

#include "EventHandler.h"
#include "Shared.h"

#include <GLFW/glfw3.h>
#include <unordered_map>

namespace Engine
{
    using InputCode = int;

    inline constexpr InputCode AnyInput = -2;

    enum class KeyCode : InputCode;
    enum class MouseKeyCode : InputCode;

    template <typename T = InputCode>
    inline constexpr InputCode WildcardCode = AnyInput;
    template <>
    inline constexpr InputCode WildcardCode<KeyCode> = AnyInput - 1;
    template <>
    inline constexpr InputCode WildcardCode<MouseKeyCode> = AnyInput - 2;

    enum class KeyCode : InputCode
    {
        Any = WildcardCode<KeyCode>,
        Unknown = GLFW_KEY_UNKNOWN,
        Space = GLFW_KEY_SPACE,
        Apostrophe = GLFW_KEY_APOSTROPHE,
        Comma = GLFW_KEY_COMMA,
        Minus = GLFW_KEY_MINUS,
        Period = GLFW_KEY_PERIOD,
        Slash = GLFW_KEY_SLASH,
        Num0 = GLFW_KEY_0,
        Num1 = GLFW_KEY_1,
        Num2 = GLFW_KEY_2,
        Num3 = GLFW_KEY_3,
        Num4 = GLFW_KEY_4,
        Num5 = GLFW_KEY_5,
        Num6 = GLFW_KEY_6,
        Num7 = GLFW_KEY_7,
        Num8 = GLFW_KEY_8,
        Num9 = GLFW_KEY_9,
        Semicolon = GLFW_KEY_SEMICOLON,
        Equal = GLFW_KEY_EQUAL,
        A = GLFW_KEY_A,
        B = GLFW_KEY_B,
        C = GLFW_KEY_C,
        D = GLFW_KEY_D,
        E = GLFW_KEY_E,
        F = GLFW_KEY_F,
        G = GLFW_KEY_G,
        H = GLFW_KEY_H,
        I = GLFW_KEY_I,
        J = GLFW_KEY_J,
        K = GLFW_KEY_K,
        L = GLFW_KEY_L,
        M = GLFW_KEY_M,
        N = GLFW_KEY_N,
        O = GLFW_KEY_O,
        P = GLFW_KEY_P,
        Q = GLFW_KEY_Q,
        R = GLFW_KEY_R,
        S = GLFW_KEY_S,
        T = GLFW_KEY_T,
        U = GLFW_KEY_U,
        V = GLFW_KEY_V,
        W = GLFW_KEY_W,
        X = GLFW_KEY_X,
        Y = GLFW_KEY_Y,
        Z = GLFW_KEY_Z,
        LeftBracket = GLFW_KEY_LEFT_BRACKET,
        Backslash = GLFW_KEY_BACKSLASH,
        RightBracket = GLFW_KEY_RIGHT_BRACKET,
        GraveAccent = GLFW_KEY_GRAVE_ACCENT,
        World1 = GLFW_KEY_WORLD_1,
        World2 = GLFW_KEY_WORLD_2,
        Escape = GLFW_KEY_ESCAPE,
        Enter = GLFW_KEY_ENTER,
        Tab = GLFW_KEY_TAB,
        Backspace = GLFW_KEY_BACKSPACE,
        Insert = GLFW_KEY_INSERT,
        Delete = GLFW_KEY_DELETE,
        Right = GLFW_KEY_RIGHT,
        Left = GLFW_KEY_LEFT,
        Down = GLFW_KEY_DOWN,
        Up = GLFW_KEY_UP,
        PageUp = GLFW_KEY_PAGE_UP,
        PageDown = GLFW_KEY_PAGE_DOWN,
        Home = GLFW_KEY_HOME,
        End = GLFW_KEY_END,
        CapsLock = GLFW_KEY_CAPS_LOCK,
        ScrollLock = GLFW_KEY_SCROLL_LOCK,
        NumLock = GLFW_KEY_NUM_LOCK,
        PrintScreen = GLFW_KEY_PRINT_SCREEN,
        Pause = GLFW_KEY_PAUSE,
        F1 = GLFW_KEY_F1,
        F2 = GLFW_KEY_F2,
        F3 = GLFW_KEY_F3,
        F4 = GLFW_KEY_F4,
        F5 = GLFW_KEY_F5,
        F6 = GLFW_KEY_F6,
        F7 = GLFW_KEY_F7,
        F8 = GLFW_KEY_F8,
        F9 = GLFW_KEY_F9,
        F10 = GLFW_KEY_F10,
        F11 = GLFW_KEY_F11,
        F12 = GLFW_KEY_F12,
        F13 = GLFW_KEY_F13,
        F14 = GLFW_KEY_F14,
        F15 = GLFW_KEY_F15,
        F16 = GLFW_KEY_F16,
        F17 = GLFW_KEY_F17,
        F18 = GLFW_KEY_F18,
        F19 = GLFW_KEY_F19,
        F20 = GLFW_KEY_F20,
        F21 = GLFW_KEY_F21,
        F22 = GLFW_KEY_F22,
        F23 = GLFW_KEY_F23,
        F24 = GLFW_KEY_F24,
        F25 = GLFW_KEY_F25,
        Keypad0 = GLFW_KEY_KP_0,
        Keypad1 = GLFW_KEY_KP_1,
        Keypad2 = GLFW_KEY_KP_2,
        Keypad3 = GLFW_KEY_KP_3,
        Keypad4 = GLFW_KEY_KP_4,
        Keypad5 = GLFW_KEY_KP_5,
        Keypad6 = GLFW_KEY_KP_6,
        Keypad7 = GLFW_KEY_KP_7,
        Keypad8 = GLFW_KEY_KP_8,
        Keypad9 = GLFW_KEY_KP_9,
        KeypadDecimal = GLFW_KEY_KP_DECIMAL,
        KeypadDivide = GLFW_KEY_KP_DIVIDE,
        KeypadMultiply = GLFW_KEY_KP_MULTIPLY,
        KeypadSubtract = GLFW_KEY_KP_SUBTRACT,
        KeypadAdd = GLFW_KEY_KP_ADD,
        KeypadEnter = GLFW_KEY_KP_ENTER,
        KeypadEqual = GLFW_KEY_KP_EQUAL,
        LeftShift = GLFW_KEY_LEFT_SHIFT,
        LeftControl = GLFW_KEY_LEFT_CONTROL,
        LeftAlt = GLFW_KEY_LEFT_ALT,
        LeftSuper = GLFW_KEY_LEFT_SUPER,
        RightShift = GLFW_KEY_RIGHT_SHIFT,
        RightControl = GLFW_KEY_RIGHT_CONTROL,
        RightAlt = GLFW_KEY_RIGHT_ALT,
        RightSuper = GLFW_KEY_RIGHT_SUPER,
        Menu = GLFW_KEY_MENU
    };

    enum class MouseKeyCode : InputCode
    {
        Any = WildcardCode<MouseKeyCode>,
        Button1 = GLFW_MOUSE_BUTTON_1,
        Button2 = GLFW_MOUSE_BUTTON_2,
        Button3 = GLFW_MOUSE_BUTTON_3,
        Button4 = GLFW_MOUSE_BUTTON_4,
        Button5 = GLFW_MOUSE_BUTTON_5,
        Button6 = GLFW_MOUSE_BUTTON_6,
        Button7 = GLFW_MOUSE_BUTTON_7,
        Button8 = GLFW_MOUSE_BUTTON_8,
        Left = GLFW_MOUSE_BUTTON_LEFT,
        Right = GLFW_MOUSE_BUTTON_RIGHT,
        Middle = GLFW_MOUSE_BUTTON_MIDDLE
    };

    enum KeyAction : int
    {
        Pressed = GLFW_PRESS,
        Released = GLFW_RELEASE,
        Held = GLFW_REPEAT
    };

    class InputHandler
    {
        using KeyEventHandler = EventHandler<const InputCode &, const KeyAction &>;

    public:
        using CallbackId = KeyEventHandler::CallbackId;
        using KeyCallback = KeyEventHandler::EventCallback;

    private:
        struct WildcardState { int heldKeyCount = 0; KeyEventHandler eventHandler; };
        struct KeyState { bool isHeld = false; KeyEventHandler eventHandler; };

        std::unordered_map<InputCode, KeyState> keyStates;
        std::unordered_map<InputCode, WildcardState> wildcardStates;

        [[nodiscard]] WildcardState &GetWildcardState(const InputCode &key);
        [[nodiscard]] KeyState &GetKeyState(const InputCode &key);

    public:
        void OnKeyStateChanged(const InputCode &key, const KeyAction &action, const InputCode &wildcardKey = AnyInput);
        KeyEventHandler::BindResult BindKey( KeyCallback callback, const InputCode &key, const InputCode &wildcardKey = AnyInput);
        bool UnbindKey(CallbackId callbackId, const InputCode &key, const InputCode &wildcardKey = AnyInput);
        [[nodiscard]] bool IsKeyHeld(const InputCode &key, const InputCode &wildcardKey = AnyInput) const;
        [[nodiscard]] float GetAxis(const KeyCode &positive, const KeyCode &negative) const;

        void GetAxis2(const KeyCode &right, const KeyCode &left, const KeyCode &up, const KeyCode &down, float &outX,
            float &outY) const;

        void GetAxis3(const KeyCode &right, const KeyCode &left, const KeyCode &up, const KeyCode &down,
            const KeyCode &forward, const KeyCode &back, float &outX, float &outY, float &outZ) const;

        template <typename T>
        void OnKeyStateChanged(const T &key, const KeyAction &action)
        {
            OnKeyStateChanged(static_cast<InputCode>(key), action, WildcardCode<T>);
        }

        template <typename T>
        KeyEventHandler::BindResult BindKey(KeyCallback callback, const T &key)
        {
            return BindKey(callback, static_cast<InputCode>(key), WildcardCode<T>);
        }

        template <typename T>
        bool UnbindKey(CallbackId callbackId, const T &key)
        {
            return UnbindKey(callbackId, static_cast<InputCode>(key), WildcardCode<T>);
        }

        template <typename T>
        bool IsKeyHeld(const T &key) const
        {
            return IsKeyHeld(static_cast<InputCode>(key), WildcardCode<T>);
        }
    };
}