#include "InputHandler.h"

#include <utility>

namespace Engine
{
#pragma region Private Getters
    InputHandler::WildcardState &InputHandler::GetWildcardState(const InputCode &key)
    {
        return wildcardStates.try_emplace(key).first->second;
    }

    InputHandler::KeyState &InputHandler::GetKeyState(const InputCode &key)
    {
        return keyStates.try_emplace(key).first->second;
    }

#pragma endregion

    void InputHandler::OnKeyStateChanged(const InputCode &key, const KeyAction &action, const InputCode &wildcardKey)
    {
        KeyAction eventAction = action;
        bool &isHeld = GetKeyState(key).isHeld;
        const bool wasHeld = isHeld;

        eventAction = static_cast<KeyAction>(action + (wasHeld && action == Pressed));
        isHeld = static_cast<bool>(action);
        const int heldCountChange = static_cast<int>(isHeld) - static_cast<int>(wasHeld);

        GetWildcardState(wildcardKey).heldKeyCount += heldCountChange;
        if (wildcardKey != AnyInput)
            GetWildcardState(AnyInput).heldKeyCount += heldCountChange;

        GetKeyState(key).eventHandler.Invoke(key, eventAction);
        GetWildcardState(wildcardKey).eventHandler.Invoke(key, eventAction);
        
        if (wildcardKey != AnyInput)
            GetWildcardState(AnyInput).eventHandler.Invoke(key, eventAction);
    }

    InputHandler::KeyEventHandler::BindResult InputHandler::BindKey(
        KeyCallback callback, const InputCode &key, const InputCode &wildcardKey)
    {
        return (key == wildcardKey)
            ? GetWildcardState(wildcardKey).eventHandler.BindCallback(std::move(callback))
            : GetKeyState(key).eventHandler.BindCallback(std::move(callback));
    }

    bool InputHandler::UnbindKey(CallbackId callbackId, const InputCode &key, const InputCode &wildcardKey)
    {
        if (key == wildcardKey)
            return GetWildcardState(wildcardKey).eventHandler.UnbindCallback(callbackId);

        const auto state = keyStates.find(key);
        return state != keyStates.end() && state->second.eventHandler.UnbindCallback(callbackId);
    }

    bool InputHandler::IsKeyHeld(const InputCode &key, const InputCode &wildcardKey) const
    {
        if (key == wildcardKey)
        {
            const auto state = wildcardStates.find(wildcardKey);
            return state != wildcardStates.end() && state->second.heldKeyCount > 0;
        }

        const auto state = keyStates.find(key);
        return state != keyStates.end() && state->second.isHeld;
    }

#pragma region Axis Queries
    float InputHandler::GetAxis(const KeyCode &positive, const KeyCode &negative) const
    {
        return static_cast<float>(IsKeyHeld(positive)) - static_cast<float>(IsKeyHeld(negative));
    }

    void InputHandler::GetAxis2(const KeyCode &right, const KeyCode &left, const KeyCode &up, const KeyCode &down,
        float &outX, float &outY) const
    {
        outX = GetAxis(right, left);
        outY = GetAxis(down, up);
    }

    void InputHandler::GetAxis3(const KeyCode &right, const KeyCode &left, const KeyCode &up, const KeyCode &down,
        const KeyCode &forward, const KeyCode &back, float &outX, float &outY, float &outZ) const
    {
        GetAxis2(right, left, up, down, outX, outY);
        outZ = GetAxis(forward, back);
    }
#pragma endregion
} // namespace Engine
