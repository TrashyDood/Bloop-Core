#include "DemoScene.h"

#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <random>
#include <sstream>
#include "Graphics.h"
#include "IOUtils.h"

DemoSceneLayer::DemoSceneLayer(Application &app) :
    Application::Layer(app),
    contentRootDirectory(IO::WorkingDirectory() / "content"),
    windowHost(Graphics::Window()) { }

Int32 DemoSceneLayer::OnStart()
{
    using namespace Graphics;

    LOG_INFO("Starting demo scene layer...\n");

    Graphics::Window::Config windowConfig{1920, 1080, "Bloop Core Engine", true};
    InputCallbacks inputCallbacks{
        [&](const Input::KeyCode key, const Input::KeyAction action)
        { inputHandler.OnKeyStateChanged(key, action); },
        [&](const Input::MouseKeyCode button, const Input::KeyAction action)
        { inputHandler.OnKeyStateChanged(button, action); }};

    if (!windowHost.Initialize(std::move(windowConfig), std::move(inputCallbacks)))
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

    sceneObjects.Create();
    sceneCameras.Create(
        [](auto &manager, const auto entity)
        {
            Transform *trans;
            manager.template Get<Transform>(entity, trans);
            trans->position = Vector3f(5, 0, -5);
            trans->LookAt();
        });

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

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
    LOG_INFO("Loading assets from '%s'...\n", contentRootDirectory.string().c_str());

    assetManager.EnqueueAssetLoad<Shader>(IO::Path::FromRelative("shaders", contentRootDirectory),
        [&]
        {
            LOG_SCOPE;
            LOG_INFO("Shaders finished loading.\n");
            activeShader = assetManager.GetResourceByIndex<Shader>(0);
        });
    assetManager.EnqueueAssetLoad<Mesh>(IO::Path::FromRelative("models", contentRootDirectory),
        [&]
        {
            LOG_SCOPE;
            LOG_INFO("Models finished loading.\n");
        });
    assetManager.EnqueueAssetLoad<Texture2D>(IO::Path::FromRelative("textures", contentRootDirectory),
        [&]
        {
            LOG_SCOPE;
            LOG_INFO("Textures finished loading.\n");

            activeTexture = assetManager.GetResourceByIndex<Texture2D>(0);
        });
}

Int32 DemoSceneLayer::DrawScene(const Application::Time &time)
{
    LOG_SCOPE;

    Graphics::Mesh *mesh = assetManager.GetResourceByName<Graphics::Mesh>("bunny/bunny");

    if (activeShader != nullptr && activeTexture != nullptr && mesh != nullptr)
    {
        activeShader->Use();
        activeTexture->Bind(GL_TEXTURE0);
        activeShader->SetUniformValue("textureSampler", Int32{0});

        const auto &cameras = sceneCameras.GetComponentList<Camera>();
        const auto &cameraTransforms = sceneCameras.GetComponentList<Transform>();
        const auto &objectTransforms = sceneObjects.GetComponentList<Transform>();

        windowHost.RenderViewports(static_cast<UInt32>(cameras.size()),
            [&](const Graphics::Window::Viewport &viewport)
            {
                activeShader->SetUniformValue(
                    "projectionMatrix", cameras[viewport.index].ProjectionMatrix(viewport.aspectRatio));
                activeShader->SetUniformValue("viewMatrix", cameraTransforms[viewport.index].GetLookMatrix());

                for (const auto &transform : objectTransforms)
                {
                    activeShader->SetUniformValue("modelMatrix", transform.GetTransformMatrix());
                    mesh->Draw();
                }
            });
    }

    DrawGUI(time);
    
    Int32 framebufferWidth = 0, framebufferHeight = 0;
    if (windowHost.GetFrameBufferSize(framebufferWidth, framebufferHeight))
        glViewport(0, 0, framebufferWidth, framebufferHeight);

    windowHost.PresentFrame();
    return EXIT_SUCCESS;
}

Int32 DemoSceneLayer::DrawGUI(const Application::Time &time)
{
    ImGuiUtils::Context::Start();
    logWindow.Draw();
    // sceneContextWindow.Draw(time.unscaledDeltaTime, time.unscaledElapsedTime, time.elapsedFrames,
    //     currentSceneName, sceneFilePaths, [&](const std::string &sceneName) {  });
    ImGuiUtils::Context::End();

    return EXIT_SUCCESS;
}

Int32 DemoSceneLayer::Update(const Application::Time &time)
{
    return EXIT_SUCCESS;
}

DemoSceneLayer::~DemoSceneLayer()
{

}