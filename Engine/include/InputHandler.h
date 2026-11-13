#pragma once

#include "EventHandler.h"
#include "InputCodes.h"

#include <unordered_map>

namespace Engine
{
    using InputCode = ::InputCode;
    using ::AnyInput;
    using ::Held;
    using ::KeyAction;
    using ::KeyCode;
    using ::MouseKeyCode;
    using ::Pressed;
    using ::Released;
    using ::WildcardCode;

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