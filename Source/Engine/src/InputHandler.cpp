#include "InputHandler.h"

#include <utility>

namespace Engine
{
#pragma region Private Getters
    InputHandler::WildcardState &InputHandler::GetWildcardState(const Input::InputCode &key)
    {
        return wildcardStates.try_emplace(key).first->second;
    }

    InputHandler::KeyState &InputHandler::GetKeyState(const Input::InputCode &key)
    {
        return keyStates.try_emplace(key).first->second;
    }

#pragma endregion

    void InputHandler::OnKeyStateChanged(
        const Input::InputCode &key, const KeyAction &action, const Input::InputCode &wildcardKey)
    {
        KeyAction eventAction = action;
        bool &isHeld = GetKeyState(key).isHeld;
        const bool wasHeld = isHeld;

        eventAction = static_cast<KeyAction>(action + (wasHeld && action == Pressed));
        isHeld = static_cast<bool>(action);
        const Int32 heldCountChange = static_cast<Int32>(isHeld) - static_cast<Int32>(wasHeld);

        GetWildcardState(wildcardKey).heldKeyCount += heldCountChange;
        if (wildcardKey != Input::AnyInput)
            GetWildcardState(Input::AnyInput).heldKeyCount += heldCountChange;

        GetKeyState(key).eventHandler.Invoke(key, eventAction);
        GetWildcardState(wildcardKey).eventHandler.Invoke(key, eventAction);
        
        if (wildcardKey != Input::AnyInput)
            GetWildcardState(Input::AnyInput).eventHandler.Invoke(key, eventAction);
    }

    InputHandler::KeyEventHandler::BindResult InputHandler::BindKey(
        KeyCallback callback, const Input::InputCode &key, const Input::InputCode &wildcardKey)
    {
        return (key == wildcardKey)
            ? GetWildcardState(wildcardKey).eventHandler.BindCallback(std::move(callback))
            : GetKeyState(key).eventHandler.BindCallback(std::move(callback));
    }

    bool InputHandler::UnbindKey(
        CallbackId callbackId, const Input::InputCode &key, const Input::InputCode &wildcardKey)
    {
        if (key == wildcardKey)
            return GetWildcardState(wildcardKey).eventHandler.UnbindCallback(callbackId);

        const auto state = keyStates.find(key);
        return state != keyStates.end() && state->second.eventHandler.UnbindCallback(callbackId);
    }

    bool InputHandler::IsKeyHeld(
        const Input::InputCode &key, const Input::InputCode &wildcardKey) const
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
    float InputHandler::GetAxis(const Input::KeyCode &positive, const Input::KeyCode &negative) const
    {
        return static_cast<float>(IsKeyHeld(positive)) - static_cast<float>(IsKeyHeld(negative));
    }

    void InputHandler::GetAxis2(
        const Input::KeyCode &right, const Input::KeyCode &left, const Input::KeyCode &up,
        const Input::KeyCode &down, float &outX, float &outY) const
    {
        outX = GetAxis(right, left);
        outY = GetAxis(down, up);
    }

    void InputHandler::GetAxis3(
        const Input::KeyCode &right, const Input::KeyCode &left, const Input::KeyCode &up,
        const Input::KeyCode &down, const Input::KeyCode &forward, const Input::KeyCode &back,
        float &outX, float &outY, float &outZ) const
    {
        GetAxis2(right, left, up, down, outX, outY);
        outZ = GetAxis(forward, back);
    }
#pragma endregion
} // namespace Engine
