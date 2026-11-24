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
#include "Transform.h"
#include "AssetResourceManager.h"

struct SceneObject
{
    Engine::Transform transform;
    std::string meshName;
};

struct SceneCamera
{
    Engine::Transform transform;
    float nearPlane = 0.1f, farPlane = 100.f, fov = 90.f;

    Matrix4f ViewMatrix()
    {
        return Engine::GetMatrix_Look(
            transform.GetPosition(),
            transform.GetForward(),
            transform.GetUp());
    }

    Matrix4f ProjectionMatrix(const float aspect)
    {
        return Graphics::GetMatrix_Perspective(
            fov,
            aspect,
            nearPlane,
            farPlane);
    }
};

struct PlayerObject
{
    UInt32 playerId = 0;
    std::shared_ptr<SceneObject> sceneObject;
    std::shared_ptr<SceneCamera> sceneCamera;
    Vector3f cameraOffset{0.f};

    void Update(const Application::Time &time, const Engine::InputHandler &inputHandler)
    {
        if (sceneObject == nullptr || sceneCamera == nullptr)
            return;

        float moveX = 0.f;
        float moveZ = 0.f;
        float rotate = 0.f;
        switch (playerId)
        {
        case 0:
            moveX = inputHandler.GetAxis(Engine::KeyCode::D, Engine::KeyCode::A);
            moveZ = inputHandler.GetAxis(Engine::KeyCode::S, Engine::KeyCode::W);
            rotate = inputHandler.GetAxis(Engine::KeyCode::E, Engine::KeyCode::Q);
            break;
        case 1:
            moveX = inputHandler.GetAxis(Engine::KeyCode::Keypad6, Engine::KeyCode::Keypad4);
            moveZ = inputHandler.GetAxis(Engine::KeyCode::Keypad2, Engine::KeyCode::Keypad8);
            rotate = inputHandler.GetAxis(Engine::KeyCode::Keypad9, Engine::KeyCode::Keypad7);
            break;
        default:
            return;
        }

        sceneObject->transform.RotateEuler({0.f, rotate * 1.5f * time.deltaTime, 0.f});

        Vector3f movement{moveX, 0.f, moveZ};
        const float inputLength = glm::length(movement);
        if (inputLength > 1.f)
            movement /= inputLength;

        movement *= 5.f * time.deltaTime;
        sceneObject->transform.TranslateLocal(movement);
        sceneCamera->transform.SetPosition(sceneObject->transform.TransformPoint(cameraOffset, false));
        sceneCamera->transform.SetLookAtPoint(sceneObject->transform.GetPosition());
    }

    void SetPosition(const Vector3f &newPosition)
    {
        sceneObject->transform.SetPosition(newPosition);
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
    
    std::vector<Vector3f> playerSpawnLocations, goalLocations;

    Graphics::Window windowHost;
    Engine::InputHandler inputHandler;

    Graphics::ShaderResource *activeShader = nullptr;
    Graphics::Texture2DResource *activeTexture = nullptr;

    AssetResourceManager assetManager;
    ImGuiUtils::Context imguiContext;
    ImGuiUtils::LogWindow logWindow;
    ImGuiUtils::SceneDiagnosticWindow diagnosticsWindow {
        assetManager.ShaderNames(),
        assetManager.MeshNames(),
        assetManager.TextureNames()};

    void BindInput();
    void StartLoadingAssets();
    void LoadNewScene(const std::string &sceneName);

    std::vector<std::shared_ptr<SceneCamera>> sceneCameras;
    std::vector<std::shared_ptr<SceneObject>> sceneObjects;
    std::vector<std::shared_ptr<PlayerObject>> scenePlayers;

    std::shared_ptr<SceneCamera> SpawnCamera(const Engine::Transform &transform, float fov, float nearPlane, float farPlane);
    std::shared_ptr<SceneObject> SpawnSceneObject(const Engine::Transform &transform, const std::string &meshName);
    std::shared_ptr<PlayerObject> SpawnPlayer(const Engine::Transform &transform);
    void SpawnPlayerAtRandomPosition();
    void RemovePlayer();

    Int32 DrawScene(const Application::Time &time);
    Int32 DrawGUI(const Application::Time &time);
    Int32 Update(const Application::Time &time);
};