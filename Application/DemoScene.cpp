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

    bool DiscoverSceneFiles(const std::filesystem::path &rootDirectory, std::vector<IO::Path> &outPaths)
    {
        outPaths.clear();

        const std::regex sceneFilePattern(R"(.*\.scene)");
        for (const auto &scenePath : IO::FindMatchingFiles_Parallel(rootDirectory, sceneFilePattern))
            outPaths.emplace_back(IO::Path::FromAbsolute(scenePath, rootDirectory));

        return outPaths.size() > 0;
    }

    Int32 LoadSceneObjects(const std::filesystem::path &scenePath,
        std::vector<std::shared_ptr<SceneObject>> &outObjects,
        std::vector<Vector3f> &outPlayerSpawnLocations,
        std::vector<Vector3f> &outGoalLocations)
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

    if (!DiscoverSceneFiles(assetsRootDirectory, sceneFilePaths))
    {
        LOG_ERROR("No scene files found under '%s'.\n", assetsRootDirectory.string().c_str());
        return EXIT_FAILURE;
    }

    LoadNewScene(sceneFilePaths.front().relativePath.stem().string());

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
        });
    assetManager.EnqueueAssetLoad<Texture2DResource>(IO::Path::FromRelative("textures", assetsRootDirectory),
        [&]
        {
            LOG_SCOPE;
            LOG_INFO("Textures finished loading.\n");
            activeTexture = assetManager.GetResourceByIndex<Texture2DResource>(0);
        });
}

void DemoSceneLayer::LoadNewScene(const std::string &sceneName)
{
    LOG_SCOPE;

    std::vector<std::shared_ptr<SceneObject>> newSceneObjects;
    std::vector<Vector3f> newPlayerSpawnLocations, newGoalLocations;
    const auto scenePath = assetsRootDirectory / "scenes" / (sceneName + ".scene");
    if (LoadSceneObjects(scenePath, newSceneObjects, newPlayerSpawnLocations, newGoalLocations) != EXIT_SUCCESS)
        return;

    if (newPlayerSpawnLocations.empty())
    {
        LOG_ERROR("Scene '%s' has no player spawn positions.\n", sceneName.c_str());
        return;
    }

    const std::size_t playerCount = scenePlayers.empty() ? 1 : scenePlayers.size();
    sceneCameras.clear();
    scenePlayers.clear();
    sceneObjects.clear();

    currentSceneName = sceneName;

    sceneObjects = std::move(newSceneObjects);
    playerSpawnLocations = std::move(newPlayerSpawnLocations);
    goalLocations = std::move(newGoalLocations);

    for (std::size_t playerIndex = 0; playerIndex < playerCount; ++playerIndex)
        SpawnPlayerAtRandomPosition();

    LOG_INFO("Loaded scene '%s'.\n", sceneName.c_str());
}

std::shared_ptr<SceneCamera> DemoSceneLayer::SpawnCamera(const Engine::Transform &transform,
    float fov, float nearPlane, float farPlane)
{
    LOG_SCOPE;
    LOG_INFO("Spawning camera with FOV %.2f, near plane %.2f, far plane %.2f.\n",
        fov, nearPlane, farPlane);

    sceneCameras.emplace_back(std::make_shared<SceneCamera>());
    auto sceneCamera = sceneCameras.back();
    
    sceneCamera->transform = transform;
    sceneCamera->camera.SetPerspective(fov, nearPlane, farPlane, false);

    return sceneCamera;
}

std::shared_ptr<SceneObject> DemoSceneLayer::SpawnSceneObject(const Engine::Transform &transform,
    const std::string &meshName)
{
    LOG_SCOPE;
    LOG_INFO("Spawning scene object with mesh '%s'.\n", meshName.c_str());

    sceneObjects.emplace_back(std::make_shared<SceneObject>());
    auto sceneObject = sceneObjects.back();

    sceneObject->transform = transform;
    sceneObject->meshName = meshName;

    return sceneObject;
}

std::shared_ptr<PlayerObject> DemoSceneLayer::SpawnPlayer(const Engine::Transform &transform)
{
    LOG_SCOPE;
    LOG_INFO("Spawning player object.\n");

    scenePlayers.emplace_back(std::make_shared<PlayerObject>());
    auto player = scenePlayers.back();

    player->playerId = static_cast<UInt32>(scenePlayers.size() - 1);
    player->sceneObject = SpawnSceneObject(transform, "bunny/bunny");
    player->cameraOffset = Vectors::v3fBackward * 5.f + Vectors::v3fUp * 2.f;
    
    Engine::Transform cameraTransform(transform.TransformPoint(player->cameraOffset, false));
    cameraTransform.SetLookAtPoint(transform.GetPosition());
    player->sceneCamera = SpawnCamera(cameraTransform, 60.f, 0.1f, 100.f);

    return player;
}

void DemoSceneLayer::SpawnPlayerAtRandomPosition()
{
    LOG_SCOPE;

    if (playerSpawnLocations.empty())
    {
        LOG_ERROR("Cannot spawn a player: the current scene has no spawn positions.\n");
        return;
    }

    static std::mt19937 randomGenerator(std::random_device{}());
    std::uniform_int_distribution<std::size_t> randomSpawnIndex(0, playerSpawnLocations.size() - 1);
    SpawnPlayer(Engine::Transform(playerSpawnLocations[randomSpawnIndex(randomGenerator)]));
}

void DemoSceneLayer::RemovePlayer()
{
    LOG_SCOPE;

    if (scenePlayers.empty())
        return;

    scenePlayers.pop_back();
    sceneCameras.pop_back();
    sceneObjects.pop_back();
}

Int32 DemoSceneLayer::DrawScene(const Application::Time &time)
{
    LOG_SCOPE;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (activeShader != nullptr && activeTexture != nullptr)
    {
        GLint modelMatrixUniform, viewMatrixUniform, projectionMatrixUniform, textureSamplerUniform;
        if (GetUniforms(modelMatrixUniform, viewMatrixUniform, projectionMatrixUniform,
                textureSamplerUniform, activeShader))
        {
            activeShader->Use();
            activeTexture->Bind(GL_TEXTURE0);

            const UInt32 viewportCount = static_cast<UInt32>(sceneCameras.size());
            for (UInt32 viewportIndex = 0; viewportIndex < viewportCount; ++viewportIndex)
            {
                float aspectRatio = 0.f;
                if (!windowHost.SetSplitScreenViewport(viewportIndex, viewportCount, aspectRatio))
                {
                    LOG_ERROR("Failed to set split-screen viewport %d of %d.\n", viewportIndex, viewportCount);
                    continue;
                }

                const auto &camera = sceneCameras[viewportIndex]->camera;
                const Matrix4f viewMatrix = camera.GetViewMatrix();
                const Matrix4f projectionMatrix = camera.GetProjectionMatrix(aspectRatio);

                for (auto &object : sceneObjects)
                {
                    Graphics::MeshResource *mesh = assetManager.GetResourceByName<Graphics::MeshResource>(object.get()->meshName);
                    if (!mesh) continue;

                    const Matrix4f modelMatrix = object.get()->transform.GetTransformMatrix();

                    glUniformMatrix4fv(modelMatrixUniform, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                    glUniformMatrix4fv(viewMatrixUniform, 1, GL_FALSE, glm::value_ptr(viewMatrix));
                    glUniformMatrix4fv(projectionMatrixUniform, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
                    glUniform1i(textureSamplerUniform, 0);

                    mesh->Draw();
                }
            }
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
    diagnosticsWindow.Draw(time.unscaledDeltaTime, time.unscaledElapsedTime, time.elapsedFrames,
        currentSceneName, sceneFilePaths,
        [&](const std::string &sceneName) { LoadNewScene(sceneName); },
        [&] { SpawnPlayerAtRandomPosition(); },
        [&] { RemovePlayer(); });
    ImGuiUtils::Context::End();

    return EXIT_SUCCESS;
}

Int32 DemoSceneLayer::Update(const Application::Time &time)
{
    for (const auto &player : scenePlayers)
        player->Update(time, inputHandler);

    return EXIT_SUCCESS;
}

DemoSceneLayer::~DemoSceneLayer()
{

}