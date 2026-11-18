#include "Shared.h"
#include "Window.h"
#include "Transform.h"
#include "Camera.h"
#include "Scene.h"
#include "ResourceUtils.h"

#include "LogHandler.h"

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
#include <optional>
#include <string>
#include <utility>
#include <vector>

#ifdef IMGUI_ENABLED
#include "ImGuiUtils.h"
#endif

struct SceneObject
{
    std::size_t meshIndex;
    std::size_t offsetIndex;
    std::string name;
    bool animates = true;
};

class DemoScene : public Engine::Scene
{
    Graphics::Window &windowHost;
    Graphics::FlyingCamera camera;
    std::vector<Graphics::MeshResource> meshes;
    std::vector<Graphics::ShaderResource> shaders;
    std::vector<Graphics::Texture2DResource> textures;

    Graphics::ShaderResource *activeShader = nullptr;
    Graphics::Texture2DResource *activeTexture = nullptr;

    double lastCursorX = 0.0, lastCursorY = 0.0;
    bool orbiting = false, wireframeEnabled = false, isSimulating = true;
#ifdef IMGUI_ENABLED
    ImGuiUtils::Context imgui;
#endif

    SharedList<SceneObject> sceneObjects;
    std::vector<std::vector<Vector3f>> objectOffsets;

public:
    explicit DemoScene(Graphics::Window &windowHost) : windowHost(windowHost) {}

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

    SceneObject *SpawnSceneObject(const Vector3f &position, const std::size_t meshIndex)
    {
        if (meshIndex >= meshes.size() || meshIndex >= objectOffsets.size())
            return nullptr;

        const std::size_t offsetIndex = objectOffsets[meshIndex].size();
        objectOffsets[meshIndex].push_back(position);
        sceneObjects.push_back(std::make_shared<SceneObject>(
            SceneObject{meshIndex, offsetIndex, "Unnamed Object", true}));

        LOG_SCOPE;
        LOG_INFO("Spawned scene object at position (%.2f, %.2f, %.2f) with mesh index %zu.\n",
            position.x, position.y, position.z, meshIndex);

        return sceneObjects.back().get();
    }

    void RemoveSceneObject(const std::size_t objectIndex)
    {
        if (objectIndex >= sceneObjects.size())
            return;

        const SceneObject &object = *sceneObjects[objectIndex];
        const std::size_t meshIndex = object.meshIndex;
        const std::size_t offsetIndex = object.offsetIndex;
        auto &offsets = objectOffsets[meshIndex];
        offsets.erase(offsets.begin() + offsetIndex);

        for (const auto &sceneObject : sceneObjects)
        {
            if (sceneObject->meshIndex == meshIndex && sceneObject->offsetIndex > offsetIndex)
                --sceneObject->offsetIndex;
        }

        LOG_SCOPE;
        LOG_INFO("Removed scene object '%s' with mesh index %zu and offset index %zu.\n",
            object.name.c_str(), meshIndex, offsetIndex);

        sceneObjects.erase(sceneObjects.begin() + objectIndex);
    }

    void ClearSceneObjects()
    {
        sceneObjects.clear();
        for (auto &offsets : objectOffsets)
            offsets.clear();

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

#ifdef IMGUI_ENABLED
    void DrawSceneContextWindow()
    {
        static Vector3f spawnPosition{0.f, 0.f, 0.f};
        static Int32 spawnMeshIndex = 0;
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
                const Vector3f &objectPosition = objectOffsets[object.meshIndex][object.offsetIndex];
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
                    const bool selected = spawnMeshIndex == static_cast<Int32>(meshIndex);
                    if (ImGui::Selectable(label.c_str(), selected))
                        spawnMeshIndex = static_cast<Int32>(meshIndex);
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
        {
            ClearSceneObjects();
            for (std::size_t meshIndex = 0; meshIndex < objectOffsets.size(); ++meshIndex)
            {
                auto &offsets = objectOffsets[meshIndex];
                offsets.clear();
            }
        }
        ImGui::EndDisabled();

        ImGui::End();
    }
#endif

    bool GetUniforms(GLint &outModelMatrixUniform, GLint &outViewMatrixUniform,
        GLint &outProjectionMatrixUniform, GLint &outTextureSamplerUniform)
    {
        LOG_SCOPE;
        if (activeShader == nullptr || !activeShader->IsInitialized())
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
                !activeShader->GetUniformLocation("textureSampler", textureSamplerLocation))
            {
                //LOG_ERROR("Required transform, camera, or texture sampler uniforms are missing from the shader program.\n");
                return false;
            }

            modelMatrixUniform =        modelLocation;
            viewMatrixUniform =         viewLocation;
            projectionMatrixUniform =   projectionLocation;
            textureSamplerUniform =     textureSamplerLocation;
            cachedShader =              activeShader;
        }

        outModelMatrixUniform =         modelMatrixUniform;
        outViewMatrixUniform =          viewMatrixUniform;
        outProjectionMatrixUniform =    projectionMatrixUniform;
        outTextureSamplerUniform =      textureSamplerUniform;
        return true;
    }

    int DrawScene(const Engine::Scene::SceneTime &time)
    {
        LOG_SCOPE;

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

        // Bind the active texture to texture unit 0 and set the shader's sampler uniform to use that unit(0).
        if (activeTexture != nullptr)
        {
            glUniform1i(textureSamplerUniform, 0);
            activeTexture->Bind(GL_TEXTURE0);
        }

        // Set the view and projection matrices based on the camera's current state.
        const Matrix4f viewMatrix = camera.GetViewMatrix(),
            projectionMatrix = camera.GetProjectionMatrix(windowHost.GetWindowAspectRatio());

        glUniformMatrix4fv(viewMatrixUniform, 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(projectionMatrixUniform, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

        const Matrix4f modelMatrix{1.f};
        glUniformMatrix4fv(modelMatrixUniform, 1, GL_FALSE, glm::value_ptr(modelMatrix));
        glPolygonMode(GL_FRONT_AND_BACK, wireframeEnabled ? GL_LINE : GL_FILL);

        for (std::size_t meshIndex = 0; meshIndex < meshes.size(); ++meshIndex)
        {
            auto &offsets = objectOffsets[meshIndex];
            if (offsets.empty())
                continue;

            if (isSimulating)
            {
                for (const auto &meshObject : sceneObjects)
                {
                    if (meshObject->meshIndex != meshIndex || !meshObject->animates)
                        continue;

                    offsets[meshObject->offsetIndex].y += glm::sin(time.elapsedTime) * time.deltaTime;
                }
            }

            meshes[meshIndex].DrawInstanced(offsets.data(), offsets.size());
        }

        return EXIT_SUCCESS;
    }

    Int32 OnStart() override
    {
        using namespace Engine;
        using namespace Graphics;

        LOG_INFO("Starting demo scene.\n");

        std::vector<std::string> shaderNames, meshNames, textureNames;
        if (!LoadAndInitResources(shaders, meshes, textures, shaderNames, meshNames, textureNames, ASSETS_DIR))
            LOG_WARNING("Some resources failed to initialize or load.\n");

        activeShader = &shaders.front();
        activeTexture = &textures.front();

        objectOffsets.resize(meshes.size());
            
        BindInput();

        camera.ResetPosition();

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

        LOG_INFO("Demo scene initialized.\n");

        return EXIT_SUCCESS;
    }

    Int32 OnMainTick(const Engine::Scene::SceneTime &time) override
    {
        LOG_SCOPE;

        int result = DrawScene(time);

#ifdef IMGUI_ENABLED
        if (!imgui.Initialize(windowHost.GetWindow()))
            return result;

        IMGUI_DRAW_START;
        DrawSceneContextWindow();
        imgui.DrawLogWindow();
        IMGUI_DRAW_END;
#endif

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

    if (!windowHost.Initialize({1200, 600, "Bloop Core Engine", true}, inputCallbacks))
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