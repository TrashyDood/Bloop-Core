#include "Shared.h"
#include "Window.h"
#include "Transform.h"
#include "Camera.h"
#include "Scene.h"
#include "ResourceLoading.h"
#include "IOUtils.h"
#include "AssetResourceManager.h"
#include "LogHandler.h"
#include "Application.h"

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

bool GetUniforms(GLint &outModelMatrixUniform, GLint &outViewMatrixUniform, GLint &outProjectionMatrixUniform,
    GLint &outTextureSamplerUniform, Graphics::ShaderResource *shader)
{
    LOG_SCOPE;
    if (shader == nullptr || !shader->IsInitialized())
    {
        // LOG_ERROR("Cannot get uniforms: no active shader is selected.\n");
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
            // LOG_ERROR("Required transform, camera, or texture sampler uniforms are missing from the shader
            // program.\n");
            return false;
        }

        modelMatrixUniform = modelLocation;
        viewMatrixUniform = viewLocation;
        projectionMatrixUniform = projectionLocation;
        textureSamplerUniform = textureSamplerLocation;
        cachedShader = shader;
    }

    outModelMatrixUniform = modelMatrixUniform;
    outViewMatrixUniform = viewMatrixUniform;
    outProjectionMatrixUniform = projectionMatrixUniform;
    outTextureSamplerUniform = textureSamplerUniform;
    return true;
}

struct SceneObject
{
    Transform transform;
    MeshAsset mesh;
};

struct SceneCamera : Graphics::Camera
{
    SceneCamera(Transform &transform) : Camera(transform) { }

    void DrawScene(const std::vector<std::shared_ptr<SceneObject>> &objects, const SceneObject *playerObject,
        Graphics::ShaderResource *shader, const float aspectRatio, const bool wireframe)
    {
        GLint modelMatrixUniform,
            viewMatrixUniform,
            projectionMatrixUniform,
            samplerUniform;

        if (!GetUniforms(modelMatrixUniform, viewMatrixUniform, projectionMatrixUniform, samplerUniform, shader))
            return;

        // Set the view and projection matrices based on the camera's current state.
        const Matrix4f viewMatrix = GetViewMatrix(),
                       projectionMatrix = GetProjectionMatrix(aspectRatio);

        glUniformMatrix4fv(viewMatrixUniform, 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(projectionMatrixUniform, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

        glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);

        for (const auto &sceneObject : objects)
        {
            if (sceneObject.get() == playerObject)
                continue;

            Graphics::MeshResource *mesh = sceneObject->mesh.GetResource();
            if (mesh == nullptr)
                continue;

            const Matrix4f modelMatrix = sceneObject->transform.GetTransformMatrix();
            glUniformMatrix4fv(modelMatrixUniform, 1, GL_FALSE, glm::value_ptr(modelMatrix));
            mesh->Draw();
        }
    }
};

struct Player : SceneObject
{
    UInt8 playerIndex = 0;
    SceneCamera camera{transform};

    void GetInputAxis(Engine::InputHandler &inputHandler, float &outRight, float &outUp, float &outForward) const
    {
        using namespace Engine;
        if (playerIndex % 2 == 0)
            inputHandler.GetAxis3(KeyCode::D, KeyCode::A, KeyCode::Q, KeyCode::E,
                KeyCode::W, KeyCode::S, outRight, outUp, outForward);
        else
            inputHandler.GetAxis3(KeyCode::Left, KeyCode::Right, KeyCode::Num0, KeyCode::Num1,
                KeyCode::Up, KeyCode::Down, outRight, outUp, outForward);
    }

    void Update(const float deltaTime, Engine::InputHandler &inputHandler)
    {
        float move, turn, elevate;
        GetInputAxis(inputHandler, turn, elevate, move);

        transform.Translate((Vectors::v3fForward * move + Vectors::v3fUp * elevate) * deltaTime);
        transform.RotateEuler(Vectors::v3fUp * turn * deltaTime);
    }
};

class DemoScene : public Engine::Scene
{
    Graphics::Window &windowHost;

    const std::filesystem::path assetsRootDirectory;
    AssetResourceManager assetManager;
    AssetHandle<Graphics::ShaderResource> activeShader;
    AssetHandle<Graphics::Texture2DResource> activeTexture;

    std::vector<std::shared_ptr<Player>> players;
    std::vector<std::shared_ptr<SceneObject>> sceneObjects;

    double lastCursorX = 0.0, lastCursorY = 0.0;
    bool orbiting = false, wireframeEnabled = false, isSimulating = true;
#ifdef IMGUI_ENABLED
    ImGuiUtils::Context imgui;
#endif

public:
    explicit DemoScene(Graphics::Window &windowHost)
        : windowHost(windowHost),
          assetsRootDirectory(IO::WorkingDirectory() / "assets") {}

    Player *SpawnPlayer(const Vector3f &position)
    {
        MeshAsset mesh(assetManager, "bunny/bunny");
        if (!mesh.IsValid())
        {
            LOG_ERROR("Failed to spawn player: the bunny mesh is unavailable.\n");
            return nullptr;
        }

        auto player = std::make_shared<Player>();
        player->playerIndex = static_cast<UInt8>(players.size());
        player->mesh = std::move(mesh);
        player->transform.SetPosition(position);
        player->transform.SetLookAtPoint(Vectors::v3fZero);

        Player *playerPointer = player.get();
        sceneObjects.push_back(player);
        players.push_back(std::move(player));
        return playerPointer;
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
                windowHost.SetVSyncEnabled(!windowHost.IsVSyncEnabled());
        }, KeyCode::V);
    }

    int DrawScene(const Engine::Scene::SceneTime &time)
    {
        LOG_SCOPE;

        using namespace Engine;
        using namespace Input;

        GLint modelMatrixUniform, viewMatrixUniform, projectionMatrixUniform, textureSamplerUniform;
        if (!GetUniforms(modelMatrixUniform, viewMatrixUniform, projectionMatrixUniform, textureSamplerUniform, activeShader.GetResource()))
            return EXIT_SUCCESS;

        activeShader.GetResource()->Use();

        // Bind the active texture to texture unit 0 and set the shader's sampler uniform to use that unit(0).
        if (Graphics::Texture2DResource *texture = activeTexture.GetResource())
        {
            glUniform1i(textureSamplerUniform, 0);
            texture->Bind(GL_TEXTURE0);
        }

        const int playerCount = static_cast<int>(players.size());
        for (int i = 0; i < playerCount; ++i)
        {
            float aspectRatio;
            if (windowHost.SetSplitScreenViewport(i, playerCount, aspectRatio))
                players[i]->camera.DrawScene(sceneObjects,
                    players[i].get(),
                    activeShader.GetResource(),
                    aspectRatio,
                    wireframeEnabled);
        }

        float fullWindowAspectRatio;
        windowHost.SetSplitScreenViewport(0, 1, fullWindowAspectRatio);

        return EXIT_SUCCESS;
    }

    void DrawGUI(const Engine::Scene::SceneTime &time)
    {
#ifdef IMGUI_ENABLED
        if (!imgui.Initialize(windowHost.GetWindow()))
            return;

        IMGUI_DRAW_START;
        ImGuiUtils::DrawSceneDiagnosticsWindow(time.unscaledDeltaTime, time.unscaledElapsedTime,
            assetManager.ShaderNames(), assetManager.MeshNames(), assetManager.TextureNames());
        imgui.DrawLogWindow();
        IMGUI_DRAW_END;
#endif
    }

    Int32 OnStart() override
    {
        using namespace Graphics;
        LOG_INFO("Starting demo scene.\n");

        assetManager.EnqueueAssetLoad<ShaderResource>(IO::Path::FromRelative("shaders", assetsRootDirectory),
            [&]
            {
                LOG_SCOPE;
                LOG_INFO("Shaders finished loading.\n");
                activeShader = AssetHandle<ShaderResource>(assetManager, "default_lit");
            });
        assetManager.EnqueueAssetLoad<MeshResource>(IO::Path::FromRelative("models", assetsRootDirectory),
            [&]
            {
                LOG_SCOPE;
                LOG_INFO("Models finished loading.\n");

                SpawnPlayer({-1.5f, 0.f, 3.f});
                SpawnPlayer({1.5f, 0.f, 3.f});
            });
        assetManager.EnqueueAssetLoad<Texture2DResource>(IO::Path::FromRelative("textures", assetsRootDirectory),
            [&]
            {
                LOG_SCOPE;
                LOG_INFO("Textures finished loading.\n");
                activeTexture = AssetHandle<Texture2DResource>(assetManager, "test");
            });

        BindInput();

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

        LOG_INFO("Demo scene initialized.\n");

        return EXIT_SUCCESS;
    }

    Int32 OnMainTick(const Engine::Scene::SceneTime &time) override
    {
        assetManager.PollPendingResources();

        for (const auto &player : players)
            player->Update(time.deltaTime, GetInput());

        const Int32 result = DrawScene(time);
        DrawGUI(time);
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