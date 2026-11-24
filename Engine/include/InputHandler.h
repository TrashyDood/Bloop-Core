#pragma once

#include "EventHandler.h"
#include "InputCodes.h"
#include "Shared.h"

#include <unordered_map>

namespace Engine
{
    using Input::AnyInput;
    using Input::InputCode;
    using Input::KeyCode;
    using Input::MouseKeyCode;
    using Input::WildcardCode;
    using Input::Held;
    using Input::KeyAction;
    using Input::Pressed;
    using Input::Released;

    class InputHandler
    {
        using KeyEventHandler = EventHandler<const Input::InputCode &, const KeyAction &>;

    public:
        using CallbackId = KeyEventHandler::CallbackId;
        using KeyCallback = KeyEventHandler::EventCallback;

    private:
        struct WildcardState { Int32 heldKeyCount = 0; KeyEventHandler eventHandler; };
        struct KeyState { bool isHeld = false; KeyEventHandler eventHandler; };

        std::unordered_map<Input::InputCode, KeyState> keyStates;
        std::unordered_map<Input::InputCode, WildcardState> wildcardStates;

        [[nodiscard]] WildcardState &GetWildcardState(const Input::InputCode &key);
        [[nodiscard]] KeyState &GetKeyState(const Input::InputCode &key);

    public:
        void OnKeyStateChanged(const Input::InputCode &key, const KeyAction &action,
            const Input::InputCode &wildcardKey = Input::AnyInput);
        KeyEventHandler::BindResult BindKey(KeyCallback callback, const Input::InputCode &key,
            const Input::InputCode &wildcardKey = Input::AnyInput);
        bool UnbindKey(CallbackId callbackId, const Input::InputCode &key,
            const Input::InputCode &wildcardKey = Input::AnyInput);

        [[nodiscard]]
        bool IsKeyHeld(const Input::InputCode &key, const Input::InputCode &wildcardKey = Input::AnyInput) const;
        [[nodiscard]]
        float GetAxis(const Input::KeyCode &positive, const Input::KeyCode &negative) const;

        void GetAxis2(const Input::KeyCode &right, const Input::KeyCode &left, const Input::KeyCode &up,
            const Input::KeyCode &down, float &outX,
            float &outY) const;

        void GetAxis3(const Input::KeyCode &right, const Input::KeyCode &left, const Input::KeyCode &up,
            const Input::KeyCode &down, const Input::KeyCode &forward, const Input::KeyCode &back,
            float &outX, float &outY, float &outZ) const;

        template <typename T>
        void OnKeyStateChanged(const T &key, const KeyAction &action)
        {
            OnKeyStateChanged(static_cast<Input::InputCode>(key), action, Input::WildcardCode<T>);
        }

        template <typename T>
        KeyEventHandler::BindResult BindKey(KeyCallback callback, const T &key)
        {
            return BindKey(callback, static_cast<Input::InputCode>(key), Input::WildcardCode<T>);
        }

        template <typename T>
        bool UnbindKey(CallbackId callbackId, const T &key)
        {
            return UnbindKey(callbackId, static_cast<Input::InputCode>(key), Input::WildcardCode<T>);
        }

        template <typename T>
        bool IsKeyHeld(const T &key) const
        {
            return IsKeyHeld(static_cast<Input::InputCode>(key), Input::WildcardCode<T>);
        }
    };
}