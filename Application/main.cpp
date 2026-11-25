#include "Shared.h"
#include "Window.h"
#include "Transform.h"
#include "Camera.h"
#include "Scene.h"
#include "ResourceLoading.h"
#include "IOUtils.h"
#include "AssetResourceManager.h"

#include "LogHandler.h"

#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#ifdef IMGUI_ENABLED
#include "ImGuiUtils.h"
#endif

using MeshAsset = AssetHandle<Graphics::MeshResource>;

struct SceneObject
{
    Transform transform;
    MeshAsset mesh;
};

struct AssetDirectories
{
    const IO::Path rootDirectory, shadersDirectory, meshesDirectory, texturesDirectory;

    AssetDirectories(const std::filesystem::path &rootDirectory)
        : rootDirectory(IO::Path::FromAbsolute(rootDirectory)),
          shadersDirectory{IO::Path::FromRelative("shaders", rootDirectory)},
          meshesDirectory{IO::Path::FromRelative("models", rootDirectory)},
          texturesDirectory{IO::Path::FromRelative("textures", rootDirectory)} {}
};

class DemoScene : public Engine::Scene
{
    Graphics::Window &windowHost;
    Graphics::FlyingCamera camera;

    AssetDirectories assetDirectories;
    AssetResourceManager assetManager;

    AssetHandle<Graphics::ShaderResource> activeShader;
    AssetHandle<Graphics::Texture2DResource> activeTexture;

    double lastCursorX = 0.0, lastCursorY = 0.0;
    bool orbiting = false, wireframeEnabled = false, isSimulating = true;
#ifdef IMGUI_ENABLED
    ImGuiUtils::Context imgui;
#endif

    SharedList<SceneObject> sceneObjects;

public:
    explicit DemoScene(Graphics::Window &windowHost)
        : windowHost(windowHost), assetDirectories(IO::WorkingDirectory() / "assets")
    {}

    void OnCursorPosition(const double x, const double y) override
    {
        if (!orbiting) { orbiting = false; return; }

        const float sensitivity = 0.005f,
            yawDelta = static_cast<float>(x - lastCursorX) * sensitivity,
            pitchDelta = static_cast<float>(lastCursorY - y) * sensitivity;

        camera.Orbit(yawDelta, pitchDelta);
        lastCursorX = x;
        lastCursorY = y;
    }

    void OnScroll(double, const double yOffset) override
    {
        constexpr float zoomSensitivity = 0.1f;
        camera.Zoom(yOffset * zoomSensitivity);
    }

    SceneObject *SpawnSceneObject(const Vector3f &position, const std::string &meshName)
    {
        MeshAsset mesh(assetManager, meshName);
        if (!mesh.IsValid())
            return nullptr;

        sceneObjects.push_back(std::make_shared<SceneObject>(
            SceneObject{Transform(position), std::move(mesh)}));

        LOG_SCOPE;
        LOG_INFO("Spawned scene object at position (%.2f, %.2f, %.2f) with mesh '%s'.\n",
            position.x, position.y, position.z, meshName.c_str());

        return sceneObjects.back().get();
    }

    void RemoveSceneObject(const std::size_t objectIndex)
    {
        if (objectIndex >= sceneObjects.size())
            return;

        LOG_SCOPE;
        LOG_INFO("Removed scene object using mesh '%s'.\n", sceneObjects[objectIndex]->mesh.Name().c_str());
        sceneObjects.erase(sceneObjects.begin() + objectIndex);
    }

    void ClearSceneObjects()
    {
        sceneObjects.clear();

        LOG_SCOPE;
        LOG_INFO("Cleared all scene objects.\n");
    }

    void BindInput()
    {
        LOG_SCOPE;
        LOG_INFO("Binding input callbacks.\n");

        using namespace Engine;

        GetInput().BindKey([this](const InputCode &, const KeyAction &action)
        {
            if (action == Pressed)
                RequestStop();
        }, KeyCode::Escape);

        GetInput().BindKey([this](const InputCode &, const KeyAction &action)
        {
            if (action == Pressed)
                wireframeEnabled = !wireframeEnabled;
        }, KeyCode::F);

        GetInput().BindKey([this](const InputCode &, const KeyAction &action)
        {
            if (action == Pressed)
                camera.SetIsOrthographic(!camera.GetIsOrthographic());
        }, KeyCode::G);

        GetInput().BindKey([this](const InputCode &, const KeyAction &action)
        {
            if (action == Pressed)
                windowHost.SetVSyncEnabled(!windowHost.IsVSyncEnabled());
        }, KeyCode::V);

        GetInput().BindKey([this](const InputCode &, const KeyAction &action)
        {
            if (action != Pressed) return;
                
            camera.ResetPosition();
            orbiting = false;
        }, KeyCode::R);

        GetInput().BindKey([this](const InputCode &, const KeyAction &action)
        {
            orbiting = action != Released;
            if (orbiting)
                windowHost.GetCursorPosition(lastCursorX, lastCursorY);
        }, MouseKeyCode::Any);
    }

    bool GetUniforms(GLint &outModelMatrixUniform, GLint &outViewMatrixUniform,
        GLint &outProjectionMatrixUniform, GLint &outTextureSamplerUniform)
    {
        LOG_SCOPE;
        Graphics::ShaderResource *shader = activeShader.GetResource();
        if (shader == nullptr || !shader->IsInitialized())
        {
            //LOG_ERROR("Cannot get uniforms: no active shader is selected.\n");
            return false;
        }

        static Graphics::ShaderResource *cachedShader = nullptr;
        static GLint modelMatrixUniform, viewMatrixUniform, projectionMatrixUniform, textureSamplerUniform;

        if (cachedShader != shader)
        {
            GLint modelLocation, viewLocation, projectionLocation, textureSamplerLocation;
            if (!shader->GetUniformLocation("modelMatrix", modelLocation) ||
                !shader->GetUniformLocation("viewMatrix", viewLocation) ||
                !shader->GetUniformLocation("projectionMatrix", projectionLocation) ||
                !shader->GetUniformLocation("textureSampler", textureSamplerLocation))
            {
                //LOG_ERROR("Required transform, camera, or texture sampler uniforms are missing from the shader program.\n");
                return false;
            }

            modelMatrixUniform =        modelLocation;
            viewMatrixUniform =         viewLocation;
            projectionMatrixUniform =   projectionLocation;
            textureSamplerUniform =     textureSamplerLocation;
            cachedShader =              shader;
        }

        outModelMatrixUniform =         modelMatrixUniform;
        outViewMatrixUniform =          viewMatrixUniform;
        outProjectionMatrixUniform =    projectionMatrixUniform;
        outTextureSamplerUniform =      textureSamplerUniform;
        return true;
    }

    int DrawScene(const Engine::Scene::SceneTime &time)
    {
        LOG_SCOPE;

        using namespace Engine;
        using namespace Input;

        assetManager.PollPendingResources();

        GLint modelMatrixUniform, viewMatrixUniform, projectionMatrixUniform, textureSamplerUniform;
        if (!GetUniforms(modelMatrixUniform, viewMatrixUniform, projectionMatrixUniform, textureSamplerUniform))
            return EXIT_SUCCESS;

        activeShader.GetResource()->Use();

        Vector3f moveVector{};
        GetInput().GetAxis3(KeyCode::D, KeyCode::A, KeyCode::Q, KeyCode::E,
            KeyCode::W, KeyCode::S, moveVector.x, moveVector.y, moveVector.z);
        camera.Move(moveVector, time.deltaTime);

        // Bind the active texture to texture unit 0 and set the shader's sampler uniform to use that unit(0).
        if (Graphics::Texture2DResource *texture = activeTexture.GetResource())
        {
            glUniform1i(textureSamplerUniform, 0);
            texture->Bind(GL_TEXTURE0);
        }

        // Set the view and projection matrices based on the camera's current state.
        const Matrix4f viewMatrix = camera.GetViewMatrix(),
            projectionMatrix = camera.GetProjectionMatrix(windowHost.GetWindowAspectRatio());

        glUniformMatrix4fv(viewMatrixUniform, 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(projectionMatrixUniform, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

        glPolygonMode(GL_FRONT_AND_BACK, wireframeEnabled ? GL_LINE : GL_FILL);

        for (const auto &sceneObject : sceneObjects)
        {
            Graphics::MeshResource *mesh = sceneObject->mesh.GetResource();
            if (mesh == nullptr)
                continue;

            if (isSimulating)
                sceneObject->transform.Translate({0.f, glm::sin(time.elapsedTime) * time.deltaTime, 0.f});

            const Matrix4f modelMatrix = sceneObject->transform.GetTransformMatrix();
            glUniformMatrix4fv(modelMatrixUniform, 1, GL_FALSE, glm::value_ptr(modelMatrix));
            mesh->Draw();
        }

        return EXIT_SUCCESS;
    }

    Int32 OnStart() override
    {
        using namespace Graphics;
        LOG_INFO("Starting demo scene.\n");

        assetManager.EnqueueAssetLoad<ShaderResource>(assetDirectories.shadersDirectory,
            [&]
            {
                LOG_SCOPE;
                LOG_INFO("Shaders finished loading.\n");
                activeShader = AssetHandle<ShaderResource>(assetManager, "default_lit");
            });
        assetManager.EnqueueAssetLoad<MeshResource>(assetDirectories.meshesDirectory,
            [&]
            {
                LOG_SCOPE;
                LOG_INFO("Models finished loading.\n");
                SpawnSceneObject({0, 0, 0}, "bunny/bunny");
            });
        assetManager.EnqueueAssetLoad<Texture2DResource>(assetDirectories.texturesDirectory,
            [&]
            {
                LOG_SCOPE;
                LOG_INFO("Textures finished loading.\n");
                activeTexture = AssetHandle<Texture2DResource>(assetManager, "test");
            });

        BindInput();

        camera.ResetPosition();

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

        LOG_INFO("Demo scene initialized.\n");

        return EXIT_SUCCESS;
    }

    Int32 OnMainTick(const Engine::Scene::SceneTime &time) override
    {
        LOG_SCOPE;

        int result = DrawScene(time);

#ifdef IMGUI_ENABLED
        if (!imgui.Initialize(windowHost.GetWindow()))
            return result;

        IMGUI_DRAW_START;
        ImGuiUtils::DrawSceneDiagnosticsWindow(time.unscaledDeltaTime, time.unscaledElapsedTime,
            camera.GetTransform().GetPosition(), camera.GetTransform().GetForward(), camera.GetOrbitDistance(),
            assetManager.ShaderNames(), assetManager.MeshNames(), assetManager.TextureNames());
        imgui.DrawLogWindow();
        IMGUI_DRAW_END;
#endif

        return result;
    }

    void OnStop(const Int32) override
    {
        LOG_SCOPE;
#ifdef IMGUI_ENABLED
        imgui.Shutdown();
#endif
    }
};

int main()
{
    using namespace Engine;
    using namespace Graphics;
    using namespace Input;

    Window windowHost;
    DemoScene demoScene(windowHost);
    InputCallbacks inputCallbacks
    {
        [&demoScene](const KeyCode key, const KeyAction action)         { demoScene.GetInput().OnKeyStateChanged(key, action); },
        [&demoScene](const MouseKeyCode button, const KeyAction action) { demoScene.GetInput().OnKeyStateChanged(button, action); },
        [&demoScene](const double x, const double y)                    { demoScene.DispatchCursorPosition(x, y); },
        [&demoScene](const double xOffset, const double yOffset)        { demoScene.DispatchScroll(xOffset, yOffset); }
    };

    if (!windowHost.Initialize({1920, 1080, "Bloop Core Engine", true}, inputCallbacks))
        return EXIT_FAILURE;

    Scene::SceneConfig sceneConfig;
    const Scene::RunHooks runHooks
    {
        [&windowHost] { windowHost.PollEvents(); },
        [&windowHost] { windowHost.PresentFrame(); },
        [&windowHost] { return !windowHost.ShouldClose(); }
    };

    return demoScene.Run(sceneConfig, runHooks);
}