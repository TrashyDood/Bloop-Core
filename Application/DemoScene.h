#pragma once

// Shared
#include "Shared.h"
#include "IOUtils.h"

// Engine
#include "Application.h"
#include "InputHandler.h"
#include "Transform.h"
#include "ECS.h"

// Graphics
#include "Graphics.h"
#include "Shader.h"
#include "Mesh.h"
#include "Texture2D.h"
#include "Window.h"
#include "ImGuiUtils.h"

#include "AssetResourceManager.h"

struct SceneObject
{
    Engine::Transform transform;
    std::string meshName;
};

struct Camera
{
    float nearPlane = 0.1f,
        farPlane = 100.f,
        fov = 90.f;

    Matrix4f ProjectionMatrix(const float aspect)
    {
        return Graphics::GetMatrix_Perspective(
            fov,
            aspect,
            nearPlane,
            farPlane);
    }
};

class DemoSceneLayer : public Application::Layer
{
public:
    DemoSceneLayer(Application &app);

    Int32 OnStart() override;
    Int32 OnTick(const Application::Time &time) override;
    void OnStop(const Int32 exitCode) override;

    ~DemoSceneLayer();

private:
    const std::filesystem::path assetsRootDirectory;
    std::vector<IO::Path> sceneFilePaths;
    std::string currentSceneName;

    Graphics::Window windowHost;
    Engine::InputHandler inputHandler;

    Graphics::Shader *activeShader = nullptr;
    Graphics::Texture2D *activeTexture = nullptr;

    AssetResourceManager assetManager;

    ImGuiUtils::Context imguiContext;
    ImGuiUtils::LogWindow logWindow;
    ImGuiUtils::SceneDiagnosticWindow sceneContextWindow {
        assetManager.ShaderNames(),
        assetManager.MeshNames(),
        assetManager.TextureNames()};

    void BindInput();
    void StartLoadingAssets();

    template <typename... TComponents>
    using SceneEntityArchetype = Engine::ECSArchetypes::LooseEntityArchetype<TComponents...>;

    SceneEntityArchetype<Engine::Transform> sceneEntities;
    SceneEntityArchetype<Engine::Transform, Camera> cameraEntities;

    Int32 DrawScene(const Application::Time &time);
    Int32 DrawGUI(const Application::Time &time);
    Int32 Update(const Application::Time &time);
};