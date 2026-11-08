#include "Rendering/Camera.h"

#include <cmath>
#include <exception>
#include <iostream>
#include <string>
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
        Require(NearlyEqual(transform.TransformPosition({1.0f, 1.0f, 1.0f}),
                            Vector3f{4.0f, 6.0f, 8.0f}),
                "TransformPosition applies scale and translation");
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

    using Test = std::pair<const char *, void (*)()>;
}

int main()
{
    const std::vector<Test> tests{
        {"transform defaults and composition", TestTransformDefaultsAndComposition},
        {"transform rotation", TestTransformRotation},
        {"orthographic camera projection", TestCameraOrthographicProjection},
        {"perspective camera projection", TestCameraPerspectiveProjection},
        {"camera view matrix", TestCameraViewMatrix},
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
