#include "DemoScene.h"

#include <glm/gtc/type_ptr.hpp>
#include "Graphics.h"

namespace // Local helpers
{
    bool GetUniforms(GLint &outModelMatrixUniform, GLint &outViewMatrixUniform, GLint &outProjectionMatrixUniform,
        GLint &outTextureSamplerUniform, Graphics::ShaderResource *shader)
    {
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
} // namespace

DemoSceneLayer::DemoSceneLayer(Application &app) :
    Application::Layer(app),
    assetsRootDirectory(IO::WorkingDirectory() / "assets"),
    windowHost(Graphics::Window()) { }

Int32 DemoSceneLayer::OnStart()
{
    using namespace Graphics;

    LOG_INFO("Starting demo scene layer...\n");

    InputCallbacks inputCallbacks{
        [&](const Input::KeyCode key, const Input::KeyAction action)
        {
            inputHandler.OnKeyStateChanged(key, action);
        },
        [&](const Input::MouseKeyCode button, const Input::KeyAction action)
        {
            inputHandler.OnKeyStateChanged(button, action);
        }};

    if (!windowHost.Initialize({1920, 1080, "Bloop Core Engine", true}, std::move(inputCallbacks)))
    {
        LOG_ERROR("Failed to initialize window for demo scene.\n");
        return EXIT_FAILURE;
    }

    if (!imguiContext.Initialize(windowHost.GetWindow()))
    {
        LOG_ERROR("Failed to initialize ImGui for demo scene.\n");
        return EXIT_FAILURE;
    }

    BindInput();
    StartLoadingAssets();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    cameraTransform.SetPosition({0.f, 0.f, 3.f});
    cameraTransform.SetLookAtPoint(Vectors::v3fZero);

    return EXIT_SUCCESS;
}

Int32 DemoSceneLayer::OnTick(const Application::Time &time)
{
    LOG_SCOPE;

    windowHost.PollEvents();
    if (windowHost.ShouldClose())
    {
        GetApplication().RequestStop();
        return EXIT_SUCCESS;
    }

    assetManager.PollPendingResources();

    Update(time);
    DrawScene(time);

    return EXIT_SUCCESS;
}

void DemoSceneLayer::OnStop(const Int32 exitCode)
{
    LOG_SCOPE;
    imguiContext.Shutdown();
}

void DemoSceneLayer::BindInput()
{
    LOG_SCOPE;
    LOG_INFO("Binding input callbacks for demo scene.\n");

    inputHandler.BindKey(
        [&](const Input::InputCode &key, const Input::KeyAction &action)
        {
            if (action != Input::KeyAction::Pressed) return;

            windowHost.Shutdown();
        },
        Input::KeyCode::Escape);
}

void DemoSceneLayer::StartLoadingAssets()
{
    using namespace Graphics;

    LOG_SCOPE;
    LOG_INFO("Loading assets from '%s'...\n", assetsRootDirectory.string().c_str());

    assetManager.EnqueueAssetLoad<ShaderResource>(IO::Path::FromRelative("shaders", assetsRootDirectory),
        [&]
        {
            LOG_SCOPE;
            LOG_INFO("Shaders finished loading.\n");
            activeShader = assetManager.GetResourceByIndex<ShaderResource>(0);
        });
    assetManager.EnqueueAssetLoad<MeshResource>(IO::Path::FromRelative("models", assetsRootDirectory),
        [&]
        {
            LOG_SCOPE;
            LOG_INFO("Models finished loading.\n");
            activeMesh = assetManager.GetResourceByIndex<MeshResource>(0);
        });
    assetManager.EnqueueAssetLoad<Texture2DResource>(IO::Path::FromRelative("textures", assetsRootDirectory),
        [&]
        {
            LOG_SCOPE;
            LOG_INFO("Textures finished loading.\n");
            activeTexture = assetManager.GetResourceByIndex<Texture2DResource>(0);
        });
}

Int32 DemoSceneLayer::DrawScene(const Application::Time &time)
{
    LOG_SCOPE;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (activeShader != nullptr && activeMesh != nullptr && activeTexture != nullptr)
    {
        GLint modelMatrixUniform, viewMatrixUniform, projectionMatrixUniform, textureSamplerUniform;
        if (GetUniforms(modelMatrixUniform, viewMatrixUniform, projectionMatrixUniform,
                textureSamplerUniform, activeShader))
        {
            activeShader->Use();

            const Matrix4f modelMatrix = meshTransform.GetTransformMatrix();
            const Matrix4f viewMatrix = camera.GetViewMatrix();
            const Matrix4f projectionMatrix = camera.GetProjectionMatrix(windowHost.GetWindowAspectRatio());

            glUniformMatrix4fv(modelMatrixUniform, 1, GL_FALSE, glm::value_ptr(modelMatrix));
            glUniformMatrix4fv(viewMatrixUniform, 1, GL_FALSE, glm::value_ptr(viewMatrix));
            glUniformMatrix4fv(projectionMatrixUniform, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
            glUniform1i(textureSamplerUniform, 0);

            activeTexture->Bind(GL_TEXTURE0);
            activeMesh->Draw();
        }
    }

    DrawGUI(time);
    
    windowHost.PresentFrame();
    return EXIT_SUCCESS;
}

Int32 DemoSceneLayer::DrawGUI(const Application::Time &time)
{
    ImGuiUtils::Context::Start();
    logWindow.Draw();
    diagnosticsWindow.Draw(time.unscaledDeltaTime, time.unscaledElapsedTime, elapsedFrames);
    ImGuiUtils::Context::End();

    return EXIT_SUCCESS;
}

Int32 DemoSceneLayer::Update(const Application::Time &time)
{
    ++elapsedFrames;
    meshTransform.RotateEuler({0.f, 0.5f * time.deltaTime, 0.f});

    return EXIT_SUCCESS;
}

DemoSceneLayer::~DemoSceneLayer()
{

}