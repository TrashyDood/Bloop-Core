#include "InputHandler.h"

#include <utility>

namespace Engine
{
    InputHandler::KeyEventHandler &InputHandler::GetKeyEventHandler(const InputCode &key)
    {
        return keyEvents.try_emplace(key).first->second;
    }

    bool &InputHandler::GetKeyHoldState(const InputCode &key)
    {
        return heldKeys.try_emplace(key).first->second;
    }

    void InputHandler::OnKeyStateChanged(const InputCode &key, const KeyAction &action)
    {
        KeyAction eventAction = action;
        bool &isHeld = GetKeyHoldState(key);
        const bool wasHeld = isHeld;

        switch (action)
        {
        case Pressed:
            eventAction = isHeld ? Held : Pressed;
            isHeld = true;
            break;
        case Held:
            isHeld = true;
            break;
        case Released:
            isHeld = false;
            break;
        }

        heldKeyCount += static_cast<int>(isHeld) - static_cast<int>(wasHeld);
        GetKeyEventHandler(key).Invoke(key, eventAction);
        anyKeyEventHandler.Invoke(key, eventAction);
    }

    InputHandler::KeyEventHandler::BindResult InputHandler::BindKeyEvent(
        KeyCallback callback, const InputCode &key)
    {
        return (key == AnyInput) ? anyKeyEventHandler.BindCallback(std::move(callback))
                                 : GetKeyEventHandler(key).BindCallback(std::move(callback));
    }

    bool InputHandler::UnbindKeyEvent(CallbackId callbackId, const InputCode &key)
    {
        if (key == AnyInput)
            return anyKeyEventHandler.UnbindCallback(callbackId);

        const auto handler = keyEvents.find(key);
        return handler != keyEvents.end() && handler->second.UnbindCallback(callbackId);
    }

    bool InputHandler::IsKeyHeld(const InputCode &key) const
    {
        if (key == AnyInput) return heldKeyCount > 0;

        const auto state = heldKeys.find(key);
        return state != heldKeys.end() && state->second;
    }

    bool InputHandler::IsKeyHeld(const KeyCode &key) const
    {
        return IsKeyHeld(static_cast<InputCode>(key));
    }

    bool InputHandler::IsKeyHeld(const MouseKey &key) const
    {
        return IsKeyHeld(static_cast<InputCode>(key));
    }

    Vector3f InputHandler::GetAxis3(KeyCode right, KeyCode left, KeyCode up, KeyCode down) const
    {
        const float x = static_cast<float>(IsKeyHeld(right)) - static_cast<float>(IsKeyHeld(left));
        const float z = static_cast<float>(IsKeyHeld(up)) - static_cast<float>(IsKeyHeld(down));
        return {x, 0, z};
    }
}
