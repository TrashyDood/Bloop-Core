#include "Camera.h"
#include "EventHandler.h"
#include "InputHandler.h"

#include <atomic>
#include <cmath>
#include <exception>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace
{
    constexpr float Tolerance = 1e-4f;

    bool NearlyEqual(float actual, float expected)
    {
        return std::abs(actual - expected) <= Tolerance;
    }

    bool NearlyEqual(const Vector3f &actual, const Vector3f &expected)
    {
        return NearlyEqual(actual.x, expected.x) &&
               NearlyEqual(actual.y, expected.y) &&
               NearlyEqual(actual.z, expected.z);
    }

    void Require(bool condition, const char *message)
    {
        if (!condition)
            throw std::runtime_error(message);
    }

    void TestTransformDefaultsAndComposition()
    {
        Shared::Transform transform;
        Require(NearlyEqual(transform.GetPosition(), Vector3f{0.0f}), "Transform starts at the origin");
        Require(NearlyEqual(transform.GetScale(), Vector3f{1.0f}), "Transform starts with unit scale");

        transform.SetPosition({2.0f, 3.0f, 4.0f});
        transform.SetScale({2.0f, 3.0f, 4.0f});
        Require(NearlyEqual(transform.TransformPoint({1.0f, 1.0f, 1.0f}),
                            Vector3f{4.0f, 6.0f, 8.0f}),
                "TransformPoint applies scale and translation");
        Require(NearlyEqual(transform.TransformVector({1.0f, 1.0f, 1.0f}),
                            Vector3f{2.0f, 3.0f, 4.0f}),
                "TransformVector applies scale without translation");
        Require(NearlyEqual(transform.TransformVector({1.0f, 1.0f, 1.0f}, false),
                            Vector3f{1.0f, 1.0f, 1.0f}),
                "TransformVector can omit scale");
    }

    void TestTransformRotation()
    {
        Shared::Transform transform;
        transform.SetRotation(glm::angleAxis(glm::half_pi<float>(), Vector3f{0.0f, 0.0f, 1.0f}));

        Require(NearlyEqual(transform.GetRight(), Vector3f{0.0f, 1.0f, 0.0f}),
                "Rotation updates the transform's right direction");
        Require(NearlyEqual(glm::length(transform.GetRotation()), 1.0f),
                "Transform stores a normalized rotation");
    }

    void TestTransformRejectsParallelLookDirection()
    {
        Shared::Transform transform;
        transform.SetRotation(glm::angleAxis(0.5f, Vector3f{0.0f, 1.0f, 0.0f}));
        const Quaternionf originalRotation = transform.GetRotation();

        transform.SetLookDirection({0.0f, 1.0f, 0.0f}, {0.0f, 2.0f, 0.0f});

        Require(NearlyEqual(glm::dot(originalRotation, transform.GetRotation()), 1.0f),
                "Parallel look direction and up vectors leave rotation unchanged");
    }

    void TestCameraOrthographicProjection()
    {
        Shared::Transform transform;
        Rendering::Camera camera(transform);
        camera.SetOrthographicSize(4.0f);

        const Matrix4f projection = camera.GetProjectionMatrix(2.0f);
        Require(NearlyEqual(projection[0][0], 0.25f), "Orthographic width follows aspect ratio");
        Require(NearlyEqual(projection[1][1], 0.5f), "Orthographic height follows orthographic size");
    }

    void TestCameraPerspectiveProjection()
    {
        Shared::Transform transform;
        Rendering::Camera camera(transform);
        camera.SetIsOrthographic(false);

        const Matrix4f projection = camera.GetProjectionMatrix(2.0f);
        Require(NearlyEqual(projection[0][0], 0.5f), "Perspective horizontal scale uses aspect ratio");
        Require(NearlyEqual(projection[1][1], 1.0f), "Perspective vertical scale uses field of view");
    }

    void TestCameraViewMatrix()
    {
        Shared::Transform transform;
        transform.SetPosition({0.0f, 0.0f, 3.0f});
        Rendering::Camera camera(transform);

        const Vector4f cameraPositionInView =
            camera.GetViewMatrix() * Vector4f{transform.GetPosition(), 1.0f};
        Require(NearlyEqual(cameraPositionInView.x, 0.0f) &&
                    NearlyEqual(cameraPositionInView.y, 0.0f) &&
                    NearlyEqual(cameraPositionInView.z, 0.0f),
                "View matrix maps camera position to the origin");
    }

    void Increment(int &value)
    {
        ++value;
    }

    void TestEventHandlerRegistrationAndRemoval()
    {
        EventHandler<int &> event;
        int value = 0;

        const auto callbackId = event.BindCallback(Increment);
        Require(callbackId.has_value(), "Event accepts a callback");
        Require(!event.BindCallback(Increment).has_value(), "Event rejects duplicate free-function callbacks");

        event.Invoke(value);
        Require(value == 1, "Event invokes registered callbacks");
        Require(event.UnbindCallback(*callbackId), "Event removes a callback by ID");
        Require(!event.UnbindCallback(*callbackId), "Event reports when a callback ID is not registered");

        event.Invoke(value);
        Require(value == 1, "Removed callbacks are not invoked");
    }

    void TestEventHandlerAllowsDuplicateLambdas()
    {
        EventHandler<int> event;
        int value = 0;
        const auto callback = [&value](int increment) { value += increment; };

        const auto firstId = event.BindCallback(callback);
        const auto secondId = event.BindCallback(callback);
        Require(firstId.has_value() && secondId.has_value(), "Event allows duplicate lambda callbacks");

        event.Invoke(2);
        Require(value == 4, "Event invokes each registered lambda");

        Require(event.UnbindCallback(*firstId), "Event removes one duplicate callback by ID");
        event.Invoke(3);
        Require(value == 7, "Removing one duplicate leaves the other registered");
    }

    void TestEventHandlerConcurrentInvocation()
    {
        EventHandler<int> event;
        std::atomic<int> value = 0;
        event.BindCallback([&value](int increment) { value.fetch_add(increment, std::memory_order_relaxed); });

        std::vector<std::thread> threads;
        for (int i = 0; i < 4; ++i)
        {
            threads.emplace_back([&event] {
                for (int j = 0; j < 1000; ++j)
                    event.Invoke(1);
            });
        }

        for (auto &thread : threads)
            thread.join();

        Require(value == 4000, "Event supports concurrent invocation");
    }

    void TestInputHandlerKeyStateTransitions()
    {
        Engine::InputHandler input;
        std::vector<Engine::KeyAction> actions;
        const auto callbackId = input.BindKey(
            [&actions](const Engine::InputCode &, const Engine::KeyAction &action) { actions.push_back(action); },
            Engine::KeyCode::A);

        Require(callbackId.has_value(), "Input handler accepts a key callback");
        Require(!input.IsKeyHeld(Engine::KeyCode::A), "Unseen keys are not held");

        input.OnKeyStateChanged(static_cast<Engine::InputCode>(Engine::KeyCode::A), Engine::Pressed);
        Require(input.IsKeyHeld(Engine::KeyCode::A), "Pressed key becomes held");
        input.OnKeyStateChanged(static_cast<Engine::InputCode>(Engine::KeyCode::A), Engine::Pressed);
        input.OnKeyStateChanged(static_cast<Engine::InputCode>(Engine::KeyCode::A), Engine::Held);
        input.OnKeyStateChanged(static_cast<Engine::InputCode>(Engine::KeyCode::A), Engine::Released);
        Require(!input.IsKeyHeld(Engine::KeyCode::A), "Released key is no longer held");
        input.OnKeyStateChanged(static_cast<Engine::InputCode>(Engine::KeyCode::A), Engine::Pressed);

        const std::vector<Engine::KeyAction> expected{
            Engine::Pressed, Engine::Held, Engine::Held, Engine::Released, Engine::Pressed};
        Require(actions == expected, "Input handler emits press, hold, and release transitions");
    }

    void TestInputHandlerUnbindsKeyCallback()
    {
        Engine::InputHandler input;
        int calls = 0;
        const auto callbackId = input.BindKey(
            [&calls](const Engine::InputCode &, const Engine::KeyAction &) { ++calls; },
            Engine::KeyCode::B);

        Require(callbackId.has_value(), "Input handler returns a callback ID");
        Require(!input.UnbindKey(*callbackId, Engine::KeyCode::A), "Callback cannot be removed from another key");
        Require(input.UnbindKey(*callbackId, Engine::KeyCode::B), "Input handler removes callback by key and ID");
        Require(!input.UnbindKey(*callbackId, Engine::KeyCode::B), "Removed callback cannot be removed twice");

        input.OnKeyStateChanged(static_cast<Engine::InputCode>(Engine::KeyCode::B), Engine::Pressed);
        Require(calls == 0, "Unbound input callback is not invoked");
    }

    void TestInputHandlerMouseBindingAndState()
    {
        Engine::InputHandler input;
        std::vector<std::pair<Engine::InputCode, Engine::KeyAction>> events;
        const auto callbackId = input.BindKey(
            [&events](const Engine::InputCode &code, const Engine::KeyAction &action)
            {
                events.emplace_back(code, action);
            },
            Engine::MouseKey::Left);

        Require(callbackId.has_value(), "Input handler accepts a typed mouse-button callback");
        Require(!input.IsKeyHeld(Engine::MouseKey::Left), "Unseen mouse buttons are not held");

        input.OnKeyStateChanged(static_cast<Engine::InputCode>(Engine::MouseKey::Left), Engine::Pressed);
        Require(input.IsKeyHeld(Engine::MouseKey::Left), "Pressed mouse button becomes held");
        input.OnKeyStateChanged(static_cast<Engine::InputCode>(Engine::MouseKey::Left), Engine::Released);
        Require(!input.IsKeyHeld(Engine::MouseKey::Left), "Released mouse button is no longer held");
        Require(events.size() == 2, "Mouse callback receives press and release events");
        Require(events[0] == std::make_pair(static_cast<Engine::InputCode>(Engine::MouseKey::Left), Engine::Pressed) &&
                    events[1] == std::make_pair(static_cast<Engine::InputCode>(Engine::MouseKey::Left), Engine::Released),
                "Mouse callback receives the button code and actions");

        Require(input.UnbindKey(*callbackId, Engine::MouseKey::Left), "Mouse callback can be unbound by typed button");
    }

    void TestInputHandlerAnyInputBinding()
    {
        Engine::InputHandler input;
        std::vector<Engine::InputCode> codes;
        const auto callbackId = input.BindKeyEvent(
            [&codes](const Engine::InputCode &code, const Engine::KeyAction &) { codes.push_back(code); });

        Require(callbackId.has_value(), "Input handler accepts an any-input callback");
        input.OnKeyStateChanged(static_cast<Engine::InputCode>(Engine::KeyCode::A), Engine::Pressed);
        input.OnKeyStateChanged(static_cast<Engine::InputCode>(Engine::MouseKey::Right), Engine::Pressed);

        Require(codes == std::vector<Engine::InputCode>{
                             static_cast<Engine::InputCode>(Engine::KeyCode::A),
                             static_cast<Engine::InputCode>(Engine::MouseKey::Right)},
                "Any-input callback receives keyboard and mouse events");
        Require(input.IsKeyHeld(Engine::AnyInput), "Any-input state is held while any input is down");

        Require(input.UnbindKeyEvent(*callbackId), "Any-input callback can be unbound");
        input.OnKeyStateChanged(static_cast<Engine::InputCode>(Engine::KeyCode::A), Engine::Released);
        input.OnKeyStateChanged(static_cast<Engine::InputCode>(Engine::MouseKey::Right), Engine::Released);
        Require(!input.IsKeyHeld(Engine::AnyInput), "Any-input state clears when all inputs are released");
    }

    void TestInputHandlerAxis3()
    {
        Engine::InputHandler input;
        input.OnKeyStateChanged(static_cast<Engine::InputCode>(Engine::KeyCode::D), Engine::Pressed);
        input.OnKeyStateChanged(static_cast<Engine::InputCode>(Engine::KeyCode::S), Engine::Pressed);

        Require(NearlyEqual(input.GetAxis3(
                                Engine::KeyCode::D, Engine::KeyCode::A,
                                Engine::KeyCode::W, Engine::KeyCode::S),
                            Vector3f{1.0f, 0.0f, -1.0f}),
                "Axis3 combines positive horizontal and negative depth input");
    }

    using Test = std::pair<const char *, void (*)()>;
}

int main()
{
    const std::vector<Test> tests{
        {"transform defaults and composition", TestTransformDefaultsAndComposition},
        {"transform rotation", TestTransformRotation},
        {"transform rejects parallel look direction", TestTransformRejectsParallelLookDirection},
        {"orthographic camera projection", TestCameraOrthographicProjection},
        {"perspective camera projection", TestCameraPerspectiveProjection},
        {"camera view matrix", TestCameraViewMatrix},
        {"event registration and removal", TestEventHandlerRegistrationAndRemoval},
        {"event duplicate lambdas", TestEventHandlerAllowsDuplicateLambdas},
        {"event concurrent invocation", TestEventHandlerConcurrentInvocation},
        {"input key state transitions", TestInputHandlerKeyStateTransitions},
        {"input callback unbinding", TestInputHandlerUnbindsKeyCallback},
        {"input mouse binding and state", TestInputHandlerMouseBindingAndState},
        {"input any-input binding", TestInputHandlerAnyInputBinding},
        {"input 3D axis", TestInputHandlerAxis3},
    };

    int failures = 0;
    for (const auto &[name, run] : tests)
    {
        try
        {
            run();
            std::cout << "[PASS] " << name << '\n';
        }
        catch (const std::exception &error)
        {
            ++failures;
            std::cerr << "[FAIL] " << name << ": " << error.what() << '\n';
        }
    }

    return failures == 0 ? 0 : 1;
}
