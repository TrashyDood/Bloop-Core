#pragma once

// Shared
#include "Shared.h"
#include "IOUtils.h"

// Engine
#include "Application.h"
#include "InputHandler.h"

// Graphics
#include "Graphics.h"
#include "ShaderResource.h"
#include "MeshResource.h"
#include "Texture2DResource.h"
#include "Window.h"
#include "ImGuiUtils.h"
#include "Camera.h"

#include "AssetResourceManager.h"

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
    
    Graphics::Window windowHost;

    AssetResourceManager assetManager;

    ImGuiUtils::Context imguiContext;
    ImGuiUtils::LogWindow logWindow;
    ImGuiUtils::SceneDiagnosticWindow diagnosticsWindow {
        assetManager.ShaderNames(),
        assetManager.MeshNames(),
        assetManager.TextureNames()};

    std::vector<Graphics::ShaderResource> shaders;
    std::vector<Graphics::MeshResource> meshes;
    std::vector<Graphics::Texture2DResource> textures;

    Engine::InputHandler inputHandler;

    Graphics::ShaderResource *activeShader = nullptr;
    Graphics::MeshResource *activeMesh = nullptr;
    Graphics::Texture2DResource *activeTexture = nullptr;
    UInt64 elapsedFrames = 0;

    Engine::Transform cameraTransform{Vectors::v3fBackward * 5.f}, meshTransform;
    Graphics::Camera camera{cameraTransform};

    void BindInput();
    void StartLoadingAssets();

    Int32 DrawScene(const Application::Time &time);
    Int32 DrawGUI(const Application::Time &time);
    Int32 Update(const Application::Time &time);
};