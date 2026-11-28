#pragma once

// Shared
#include "Shared.h"
#include "IOUtils.h"

// Engine
#include "Application.h"
#include "InputHandler.h"
#include "ECS.h"

// Graphics
#include "Graphics.h"
#include "Shader.h"
#include "Mesh.h"
#include "Texture2D.h"
#include "Window.h"
#include "ImGuiUtils.h"

#include "AssetResourceManager.h"

struct Camera
{
    float nearPlane = 0.1f,
        farPlane = 100.f,
        fov = 90.f;

    Matrix4f ProjectionMatrix(const float aspect) const
    {
        return Graphics::GetMatrix_Perspective(
            fov,
            aspect,
            nearPlane,
            farPlane);
    }
};

struct Transform
{
    Vector3f position, scale;
    Quaternion rotation;

    Matrix4f GetTransformMatrix() const
    {
        return Matrices::GetMatrix_Transform(position, Matrices::GetMatrix_Rotation(rotation));
    }

    Matrix4f GetLookMatrix() const
    {
        const Matrix4f transform = GetTransformMatrix();
        return Matrices::GetMatrix_Look(position,
            Matrices::TransformVector(transform, Vectors::v3fForward),
            Matrices::TransformVector(transform, Vectors::v3fUp));
    }

    void LookAt(const Vector3f &target = Vectors::v3fZero) { rotation = Quaternions::LookAt(target - position); }
};

struct TransformMatrix { Matrix4f value{1.f}; };

class DemoSceneLayer : public Application::Layer
{
public:
    DemoSceneLayer(Application &app);

    Int32 OnStart() override;
    Int32 OnTick(const Application::Time &time) override;
    void OnStop(const Int32 exitCode) override;

    ~DemoSceneLayer();

private:
    const std::filesystem::path contentRootDirectory;

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

    template <typename... TComps>
    class SceneEntityArchetype : public Engine::ECSArchetypes::EntityArchetype<Transform, TransformMatrix, TComps...>
    {
        template <typename TArchetype>
        void UpdateTransformMatrices(TArchetype &archetype)
        {
            auto &transforms = archetype.template GetComponents<Transform>();
            auto &matrices = archetype.template GetComponents<TransformMatrix>();

            for (std::size_t index = 0; index < archetype.Count(); ++index)
                matrices[index].value = transforms[index].GetTransformMatrix();
        }
    };

    SceneEntityArchetype<> sceneObjects;
    SceneEntityArchetype<Camera> sceneCameras;

    Int32 DrawScene(const Application::Time &time);
    Int32 DrawGUI(const Application::Time &time);
    Int32 Update(const Application::Time &time);
};