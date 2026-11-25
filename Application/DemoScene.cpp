#include "DemoScene.h"

#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <random>
#include <sstream>
#include "Graphics.h"
#include "IOUtils.h"

namespace // Local helpers
{
    bool GetUniforms(GLint &outModelMatrixUniform, GLint &outViewMatrixUniform, GLint &outProjectionMatrixUniform,
        GLint &outTextureSamplerUniform, Graphics::Shader *shader)
    {
        if (shader == nullptr || !shader->IsInitialized())
        {
            // LOG_ERROR("Cannot get uniforms: no active shader is selected.\n");
            return false;
        }

        static Graphics::Shader *cachedShader = nullptr;
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

    bool DiscoverSceneFiles(const std::filesystem::path &rootDirectory, std::vector<IO::Path> &outPaths)
    {
        outPaths.clear();

        for (const auto &scenePath : IO::FindMatchingFiles_Parallel(rootDirectory, std::regex(R"(.*\.scene)")))
            outPaths.emplace_back(IO::Path::FromAbsolute(scenePath, rootDirectory));

        return outPaths.size() > 0;
    }

    Int32 LoadSceneObjects(const std::filesystem::path &scenePath, std::vector<std::shared_ptr<SceneObject>> &outObjects,
        std::vector<Vector3f> &outPlayerSpawnLocations, std::vector<Vector3f> &outGoalLocations)
    {
        LOG_SCOPE;

        std::ifstream sceneFile(scenePath);
        if (!sceneFile)
        {
            LOG_ERROR("Failed to open scene file '%s'.\n", scenePath.string().c_str());
            return EXIT_FAILURE;
        }

        std::string line;
        UInt32 lineNumber = 0;
        while (std::getline(sceneFile, line))
        {
            ++lineNumber;

            std::istringstream lineStream(line);
            std::string recordType, meshName, extra;
            Vector3f position;
            if (!(lineStream >> recordType))
                continue;

            if (recordType == "object:")
            {
                if (!(lineStream >> meshName))
                {
                    LOG_ERROR("Invalid scene record at line %u in '%s'.\n",
                        lineNumber, scenePath.string().c_str());
                    return EXIT_FAILURE;
                }
            }
            else if (recordType != "spawnposition" && recordType != "goalposition")
            {
                LOG_ERROR("Invalid scene record at line %u in '%s'.\n",
                    lineNumber, scenePath.string().c_str());
                return EXIT_FAILURE;
            }

            if (!(lineStream >> position.x >> position.y >> position.z) || (lineStream >> extra))
            {
                LOG_ERROR("Invalid scene record at line %u in '%s'.\n",
                    lineNumber, scenePath.string().c_str());
                return EXIT_FAILURE;
            }

            if (recordType == "object:")
            {
                auto sceneObject = std::make_shared<SceneObject>();
                sceneObject->transform = Engine::Transform(position);
                sceneObject->meshName = meshName;
                outObjects.emplace_back(std::move(sceneObject));
            }
            else if (recordType == "spawnposition")
                outPlayerSpawnLocations.emplace_back(position);
            else
                outGoalLocations.emplace_back(position);
        }

        if (sceneFile.bad())
        {
            LOG_ERROR("Failed while reading scene file '%s'.\n", scenePath.string().c_str());
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
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

    sceneEntities.Add();
    sceneEntities.Add();
    cameraEntities.Add<Engine::Transform, Camera>({Vectors::v3fBackward * 5.f}, {});

    Camera *camera;
    Engine::Transform *transform;
    cameraEntities.Get(0, transform, camera);

    if (!DiscoverSceneFiles(assetsRootDirectory, sceneFilePaths))
    {
        LOG_ERROR("No scene files found under '%s'.\n", assetsRootDirectory.string().c_str());
        return EXIT_FAILURE;
    }

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
    LOG_INFO("Loading assets from '%s'...\n", assetsRootDirectory.string().c_str());

    assetManager.EnqueueAssetLoad<Shader>(IO::Path::FromRelative("shaders", assetsRootDirectory),
        [&]
        {
            LOG_SCOPE;
            LOG_INFO("Shaders finished loading.\n");
            activeShader = assetManager.GetResourceByIndex<Shader>(0);
        });
    assetManager.EnqueueAssetLoad<Mesh>(IO::Path::FromRelative("models", assetsRootDirectory),
        [&]
        {
            LOG_SCOPE;
            LOG_INFO("Models finished loading.\n");
        });
    assetManager.EnqueueAssetLoad<Texture2D>(IO::Path::FromRelative("textures", assetsRootDirectory),
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

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Graphics::Mesh *mesh = assetManager.GetResourceByName<Graphics::Mesh>("bunny/bunny");

    if (activeShader != nullptr && activeTexture != nullptr && mesh != nullptr)
    {
        GLint modelMatrixUniform, viewMatrixUniform, projectionMatrixUniform, textureSamplerUniform;
        if (GetUniforms(modelMatrixUniform, viewMatrixUniform, projectionMatrixUniform,
                textureSamplerUniform, activeShader))
        {
            activeShader->Use();
            activeTexture->Bind(GL_TEXTURE0);

            auto cameras = cameraEntities.GetComponents<Camera>();
            auto cameraTransforms = cameraEntities.GetComponents<Engine::Transform>();
            auto sceneTransforms = sceneEntities.GetComponents<Engine::Transform>();

            windowHost.RenderViewports(static_cast<UInt32>(cameraEntities.Count()),
                [&](const Graphics::WindowViewportContext &viewport)
                {                    
                    const Matrix4f projectionMatrix = cameras[viewport.index].ProjectionMatrix(viewport.aspectRatio);
                    const Matrix4f viewMatrix = cameraTransforms[viewport.index].GetLookMatrix();

                    for (auto &transform : sceneTransforms)
                    {
                        const Matrix4f modelMatrix = transform.GetTransformMatrix();

                        glUniformMatrix4fv(modelMatrixUniform, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                        glUniformMatrix4fv(viewMatrixUniform, 1, GL_FALSE, glm::value_ptr(viewMatrix));
                        glUniformMatrix4fv(projectionMatrixUniform, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
                        glUniform1i(textureSamplerUniform, 0);
                    }

                    mesh->Draw();
                });
        }
    }

    Int32 framebufferWidth = 0, framebufferHeight = 0;
    if (windowHost.GetFrameBufferSize(framebufferWidth, framebufferHeight))
        glViewport(0, 0, framebufferWidth, framebufferHeight);

    DrawGUI(time);
    
    windowHost.PresentFrame();
    return EXIT_SUCCESS;
}

Int32 DemoSceneLayer::DrawGUI(const Application::Time &time)
{
    ImGuiUtils::Context::Start();
    logWindow.Draw();
    sceneContextWindow.Draw(time.unscaledDeltaTime, time.unscaledElapsedTime, time.elapsedFrames,
        currentSceneName, sceneFilePaths, [&](const std::string &sceneName) {  });
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