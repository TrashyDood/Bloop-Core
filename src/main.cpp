#include "Shared.h"
#include "Window.h"
#include "Transform.h"
#include "Camera.h"
#include "Mesh.h"
#include "MeshLoader.h"
#include "Shader.h"
#include "Scene.h"
#include "Texture.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <stb_image.h>

#define IMGUI_DRAW_START         \
    ImGui_ImplOpenGL3_NewFrame();\
    ImGui_ImplGlfw_NewFrame();   \
    ImGui::NewFrame();

#define IMGUI_DRAW_END                                     \
    ImGui::Render();                                       \
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

struct SceneObject
{
    std::size_t meshIndex;
    Transform transform;
    std::string name;
    bool animates = true;
};

class DemoScene : public Engine::Scene
{
    Graphics::Window &windowHost;
    Graphics::FlyingCamera camera;
    std::vector<Graphics::MeshResource> meshes;
    std::vector<Graphics::ShaderResource> shaders;
    std::vector<Graphics::TextureResource> textures;

    Graphics::ShaderResource *activeShader = nullptr;
    Graphics::TextureResource *activeTexture = nullptr;

    double lastCursorX = 0.0, lastCursorY = 0.0;
    bool orbiting = false, wireframeEnabled = true, isSimulating = true;
    bool imguiInitialized = false;
    bool useCustomOBJLoader = true;

    SharedList<SceneObject> sceneObjects;

public:
    explicit DemoScene(Graphics::Window &windowHost) : windowHost(windowHost) {}

#pragma region Input Handlers
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
#pragma endregion

    SceneObject *SpawnSceneObject(const Vector3f &position, const std::size_t meshIndex)
    {
        if (meshIndex >= meshes.size())
            return nullptr;

        sceneObjects.push_back(std::make_shared<SceneObject>(
            SceneObject{meshIndex, position, "Unnamed Object"}));
        return sceneObjects.back().get();
    }

    void RemoveSceneObject(const std::size_t objectIndex)
    {
        if (sceneObjects.size() > 0)
        sceneObjects.erase(sceneObjects.begin() + objectIndex);
    }

    bool LoadAssets()
    {
        using namespace Graphics;

        // Find and load all .obj mesh files in the scene's model directory.
        for (auto &file : IOUtils::FindMatchingFiles(MODEL_DIR, std::regex(".*\\.obj")))
        {
            std::vector<std::string> objectNames;
            std::vector<MeshVertices> objectVertices;
            std::vector<MeshIndices> objectIndices;
            
            if (useCustomOBJLoader)
            {
                if (LoadMeshFile(file.string().c_str(), objectNames, objectVertices, objectIndices))
                    for (std::size_t i = 0; i < objectVertices.size(); ++i)
                        meshes.emplace_back(std::move(objectVertices[i]), std::move(objectIndices[i]));
            }
            else
            {
                if (LoadMeshFileObjLoader(file.string().c_str(), objectNames, objectVertices, objectIndices))
                    for (std::size_t i = 0; i < objectVertices.size(); ++i)
                        meshes.emplace_back(std::move(objectVertices[i]), std::move(objectIndices[i]));
            }
        }

        // Find and load all .glsl shader files in the scene's shader directory, grouping them by name.
        std::map<std::string, std::vector<ShaderSource>> shaderGroups;
        for (const auto &file : IOUtils::FindMatchingFiles(SHADER_DIR, std::regex(".*\\.glsl")))
        {
            const Graphics::ShaderType type = GetShaderFileType(file);
            if (type != Graphics::Unknown)
            {
                std::string source;
                if (!IOUtils::ReadFileToEnd(file.string().c_str(), source))
                    continue;
                shaderGroups[GetShaderGroupName(file)].push_back({std::move(source), type});
            }
        }

        for (auto &[name, sources] : shaderGroups)
            shaders.emplace_back(std::move(sources));

        if (shaders.empty())
        {
            LOG_ERROR("No supported shader files found in '%s'.\n", SHADER_DIR);
            return false;
        }

        // Find and load all .png texture files in the scene's texture directory.
        for (const auto &file : IOUtils::FindMatchingFiles(TEXTURES_DIR, std::regex(".*\\.png", std::regex::icase)))
        {
            const std::string path = file.string();
            int width = 0, height = 0;

            std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> image(
                stbi_load(path.c_str(), &width, &height, nullptr, STBI_rgb_alpha), stbi_image_free);

            if (!image)
            {
                LOG_ERROR("Failed to load texture '%s': %s\n", path.c_str(), stbi_failure_reason());
                return false;
            }

            const auto byteCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4;
            std::vector<std::uint8_t> rgba(image.get(), image.get() + byteCount);
            textures.emplace_back(width, height, std::move(rgba));
        }

        return true;
    }

    bool InitAssets()
    {
        using namespace Graphics;

        if (!InitializeResources(meshes) || !InitializeResources(shaders) || !InitializeResources(textures))
        {
            LOG_ERROR("Failed to initialize resources.\n");
            return false;
        }

        activeShader = &shaders.front();
        activeTexture = &textures.front();

        return true;
    }

    void BindInput()
    {
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

    void DrawSceneContextWindow()
    {
        static Vector3f spawnPosition{0.f, 0.f, 0.f};
        static int spawnMeshIndex = 0;
        static bool enabled = true;
        static float timeScale = GetConfig().timeScale;

        const auto drawVector = [](const Vector3f &vector)
        {
            ImGui::TextColored({1.f, 0.3f, 0.3f, 1.f}, "X %.2f", vector.x);
            ImGui::SameLine();
            ImGui::TextColored({0.3f, 1.f, 0.3f, 1.f}, "Y %.2f", vector.y);
            ImGui::SameLine();
            ImGui::TextColored({0.3f, 0.5f, 1.f, 1.f}, "Z %.2f", vector.z);
        };

        static float targetFramerate = [](const double targetTimeStep)
        {
            if (!std::isfinite(targetTimeStep) || targetTimeStep <= 0.0)
                return 30.f;

            const double framerate = 1.0 / targetTimeStep;
            if (!std::isfinite(framerate) || framerate <= 0.0 || framerate > std::numeric_limits<float>::max())
                return 30.f;

            return static_cast<float>(framerate);
        }(GetConfig().targetTimeStep.count());

        if (!imguiInitialized)
        {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGui::StyleColorsDark();
            ImGuiStyle &style = ImGui::GetStyle();
            style.Colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.f);
            style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.18f, 0.18f, 0.18f, 1.f);
            style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.12f, 0.12f, 0.12f, 0.75f);

            if (!ImGui_ImplGlfw_InitForOpenGL(&windowHost.GetWindow(), true))
            {
                LOG_ERROR("Failed to initialize ImGui GLFW backend.\n");
                ImGui::DestroyContext();
                return;
            }

            if (!ImGui_ImplOpenGL3_Init("#version 330"))
            {
                LOG_ERROR("Failed to initialize ImGui OpenGL backend.\n");
                ImGui_ImplGlfw_Shutdown();
                ImGui::DestroyContext();
                return;
            }

            imguiInitialized = true;
        }

        IMGUI_DRAW_START;
        if (!ImGui::Begin("Scene Context", &enabled))
        {
            ImGui::End();
            return;
        }

        ImGui::SeparatorText("Info");

        const float elapsedTime = GetTime().unscaledElapsedTime;
        const Vector3f &position = camera.GetTransform().GetPosition();
        const Vector3f direction = camera.GetTransform().GetForward();

        ImGui::Text("Framerate: %.1f FPS", 1.f / std::max(SMALL_NUMBER, GetTime().unscaledDeltaTime));
        ImGui::Text("Time elapsed: %02d:%05.2f", static_cast<int>(elapsedTime / 60.f), std::fmod(elapsedTime, 60.f));

        ImGui::TextUnformatted("Camera position:");
        ImGui::SameLine();
        drawVector(position);
        ImGui::TextUnformatted("Camera facing:");
        ImGui::SameLine();
        drawVector(direction);
        ImGui::Text("Camera distance: %.2f", camera.GetOrbitDistance());

        if (ImGui::Button("Reset camera (R)"))
        {
            camera.ResetPosition();
            orbiting = false;
        }

        ImGui::Spacing();
        ImGui::SeparatorText("Graphical Settings");
        bool isOrthographic = camera.GetIsOrthographic();
        if (ImGui::Checkbox("Orthographic (G)", &isOrthographic))
            camera.SetIsOrthographic(isOrthographic);
        ImGui::Checkbox("Wireframe (F)", &wireframeEnabled);

        bool vsyncEnabled = windowHost.IsVSyncEnabled();
        if (ImGui::Checkbox("VSync (V)", &vsyncEnabled))
            windowHost.SetVSyncEnabled(vsyncEnabled);

        ImGui::BeginDisabled(vsyncEnabled);
        ImGui::SetNextItemWidth(100.f);
        ImGui::InputFloat("Target framerate", &targetFramerate, 1.f, 10.f, "%.1f");
        ImGui::EndDisabled();

        if (vsyncEnabled)
            GetConfig().targetTimeStep = std::chrono::duration<double>::zero();
        else if (std::isfinite(targetFramerate) && targetFramerate > 0.f)
            GetConfig().targetTimeStep = std::chrono::duration<double>(1.0 / targetFramerate);
        else
            ImGui::TextUnformatted("Target framerate must be finite and greater than zero.");

        ImGui::Spacing();
        ImGui::SeparatorText("Simulation Settings");
        ImGui::SetNextItemWidth(100.f);
        ImGui::SliderFloat("Timescale", &timeScale, 0.f, 2.f, "%.2fx");
        ImGui::SameLine();
        ImGui::Checkbox("Is Simulating", &isSimulating);
        GetConfig().timeScale = timeScale;

        ImGui::Spacing();
        ImGui::SeparatorText("Scene Objects");
        if (ImGui::BeginChild("SceneObjectList", ImVec2(0.f, 200.f), ImGuiChildFlags_Borders,
            ImGuiWindowFlags_AlwaysVerticalScrollbar))
        {
            for (std::size_t i = 0; i < sceneObjects.size(); ++i)
            {
                ImGui::PushID(static_cast<int>(i));
                SceneObject &object = *sceneObjects[i];
                const Vector3f &objectPosition = object.transform.GetPosition();
                if (object.meshIndex < meshes.size())
                    ImGui::BulletText("%zu: Mesh %zu", i + 1, object.meshIndex + 1);
                else
                    ImGui::BulletText("%zu: Unknown", i + 1);
                ImGui::SameLine();
                ImGui::Checkbox("Animate", &object.animates);
                ImGui::SameLine();
                drawVector(objectPosition);
                ImGui::SameLine();
                if (ImGui::SmallButton("Remove"))
                {
                    RemoveSceneObject(i);
                    ImGui::PopID();
                    break;
                }
                ImGui::PopID();
            }
        }
        ImGui::EndChild();
        if (meshes.empty())
            ImGui::TextUnformatted("No meshes available");
        else
        {
            if (spawnMeshIndex < 0 || static_cast<std::size_t>(spawnMeshIndex) >= meshes.size())
                spawnMeshIndex = 0;

            const std::string preview = "Mesh " + std::to_string(spawnMeshIndex + 1);
            if (ImGui::BeginCombo("Mesh", preview.c_str()))
            {
                for (std::size_t meshIndex = 0; meshIndex < meshes.size(); ++meshIndex)
                {
                    const std::string label = "Mesh " + std::to_string(meshIndex + 1);
                    const bool selected = spawnMeshIndex == static_cast<int>(meshIndex);
                    if (ImGui::Selectable(label.c_str(), selected))
                        spawnMeshIndex = static_cast<int>(meshIndex);
                    if (selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }
        ImGui::InputFloat3("Position", glm::value_ptr(spawnPosition));
        ImGui::BeginDisabled(meshes.empty());
        if (ImGui::Button("Spawn Object"))
            SpawnSceneObject(spawnPosition, static_cast<std::size_t>(spawnMeshIndex));
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::BeginDisabled(sceneObjects.empty());
        if (ImGui::Button("Clear Objects"))
            sceneObjects.clear();
        ImGui::EndDisabled();

        ImGui::End();

        IMGUI_DRAW_END;
    }

    bool GetUniforms(
        GLint &outModelMatrixUniform, GLint &outViewMatrixUniform,
        GLint &outProjectionMatrixUniform, GLint &outTextureSamplerUniform)
    {
        if (activeShader == nullptr)
        {
            LOG_ERROR("Cannot get uniforms: no active shader is selected.\n");
            return false;
        }

        static Graphics::ShaderResource *cachedShader = nullptr;
        static GLint modelMatrixUniform, viewMatrixUniform, projectionMatrixUniform, textureSamplerUniform;

        if (cachedShader != activeShader)
        {
            GLint modelLocation, viewLocation, projectionLocation, textureSamplerLocation;
            if (!activeShader->GetUniformLocation("modelMatrix", modelLocation) ||
                !activeShader->GetUniformLocation("viewMatrix", viewLocation) ||
                !activeShader->GetUniformLocation("projectionMatrix", projectionLocation) ||
                !activeShader->GetUniformLocation("diffuseTexture", textureSamplerLocation))
            {
                LOG_ERROR("Required transform, camera, or texture sampler uniforms are missing from the shader program.\n");
                return false;
            }

            modelMatrixUniform = modelLocation;
            viewMatrixUniform = viewLocation;
            projectionMatrixUniform = projectionLocation;
            textureSamplerUniform = textureSamplerLocation;
            cachedShader = activeShader;
        }

        outModelMatrixUniform = modelMatrixUniform;
        outViewMatrixUniform = viewMatrixUniform;
        outProjectionMatrixUniform = projectionMatrixUniform;
        outTextureSamplerUniform = textureSamplerUniform;
        return true;
    }

    int OnStart() override
    {
        using namespace Engine;
        using namespace Graphics;

        if (!LoadAssets())
            return EXIT_FAILURE;

        if (!InitAssets())
            return EXIT_FAILURE;

        BindInput();

        camera.ResetPosition();

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

        return EXIT_SUCCESS;
    }

    int OnMainTick(const Engine::Scene::SceneTime &time) override
    {
        using namespace Engine;
        using namespace Input;

        GLint modelMatrixUniform, viewMatrixUniform, projectionMatrixUniform, textureSamplerUniform;
        if (!GetUniforms(modelMatrixUniform, viewMatrixUniform, projectionMatrixUniform, textureSamplerUniform))
            return EXIT_SUCCESS;

        activeShader->Use();

        Vector3f moveVector{};
        GetInput().GetAxis3(KeyCode::D, KeyCode::A, KeyCode::Q, KeyCode::E,
            KeyCode::W, KeyCode::S, moveVector.x, moveVector.y, moveVector.z);
        camera.Move(moveVector, time.deltaTime);

        glUniform1i(textureSamplerUniform, 0);
        if (activeTexture != nullptr)
            activeTexture->Bind(GL_TEXTURE0);

        // Set the view and projection matrices based on the camera's current state.
        const Matrix4f viewMatrix = camera.GetViewMatrix(),
            projectionMatrix = camera.GetProjectionMatrix(windowHost.GetWindowAspectRatio());

        glUniformMatrix4fv(viewMatrixUniform, 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(projectionMatrixUniform, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

        glPolygonMode(GL_FRONT_AND_BACK, wireframeEnabled ? GL_LINE : GL_FILL);

        for (auto &meshObject : sceneObjects)
        {
            if (meshObject->animates && isSimulating)
            {
                const float pulseScale = 1.f + glm::sin(time.elapsedTime) * 0.5f;
                meshObject->transform.RotateEuler({0.f, time.deltaTime, 0.f});
                meshObject->transform.Translate({0.f, glm::sin(time.elapsedTime) * time.deltaTime, 0.f});
                meshObject->transform.SetScale({pulseScale, pulseScale, pulseScale});
            }

            const Matrix4f modelMatrix = meshObject->transform.GetTransformMatrix();
            glUniformMatrix4fv(modelMatrixUniform, 1, GL_FALSE, glm::value_ptr(modelMatrix));
            meshes[meshObject->meshIndex].Draw();
        }

        DrawSceneContextWindow();

        return EXIT_SUCCESS;
    }

    void OnStop(const int) override
    {
        if (!imguiInitialized)
            return;

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        imguiInitialized = false;
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

    if (!windowHost.Initialize({1800, 1200, "Bloop Core Engine", true}, inputCallbacks))
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