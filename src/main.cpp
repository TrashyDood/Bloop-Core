#include "Shared.h"
#include "Transform.h"
#include "Camera.h"
#include "Mesh.h"
#include "Shader.h"
#include "Scene.h"

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
#include <string>

#define IMGUI_DRAW_START         \
    ImGui_ImplOpenGL3_NewFrame();\
    ImGui_ImplGlfw_NewFrame();   \
    ImGui::NewFrame();

#define IMGUI_DRAW_END                                     \
    ImGui::Render();                                       \
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

struct MeshObject { Rendering::MeshResource *mesh; Engine::Transform transform; bool animates; };
struct MeshGeometry { Rendering::MeshVertices vertices; Rendering::MeshIndices indices; };

#pragma region Global Constants
const MeshGeometry CubeGeometry{
    {
        {{-0.5f, -0.5f, 0.5f}, {}, {}, Colors::Red},
        {{0.5f, -0.5f, 0.5f}, {}, {}, Colors::Green},
        {{0.5f, 0.5f, 0.5f}, {}, {}, Colors::Blue},
        {{-0.5f, 0.5f, 0.5f}, {}, {}, Colors::Yellow},
        {{-0.5f, -0.5f, -0.5f}, {}, {}, Colors::Cyan},
        {{0.5f, -0.5f, -0.5f}, {}, {}, Colors::Magenta},
        {{0.5f, 0.5f, -0.5f}, {}, {}, Colors::Orange},
        {{-0.5f, 0.5f, -0.5f}, {}, {}, Colors::Purple},
    },
    {
        0, 1, 2,
        0, 2, 3,
        5, 4, 7,
        5, 7, 6,
        1, 5, 6,
        1, 6, 2,
        4, 0, 3,
        4, 3, 7,
        3, 2, 6,
        3, 6, 7,
        4, 5, 1,
        4, 1, 0
    }};

const MeshGeometry PyramidGeometry{
    {
        {{-0.5f, 0.5f, 0.5f}, {}, {}, Colors::Red},
        {{0.5f, 0.5f, 0.5f}, {}, {}, Colors::Green},
        {{0.5f, 0.5f, -0.5f}, {}, {}, Colors::Blue},
        {{-0.5f, 0.5f, -0.5f}, {}, {}, Colors::Yellow},
        {{0.f, -0.5f, 0.f}, {}, {}, Colors::White},
    },
    {
        0, 4, 1,
        1, 4, 2,
        2, 4, 3,
        3, 4, 0,
        0, 2, 3,
        0, 1, 2
    }
};
#pragma endregion

class DemoScene : public Engine::Scene
{
    Rendering::FlyingCamera camera;
    SharedList<Rendering::MeshResource> meshes{
        std::make_shared<Rendering::MeshResource>(CubeGeometry.vertices, CubeGeometry.indices),
        std::make_shared<Rendering::MeshResource>(PyramidGeometry.vertices, PyramidGeometry.indices)
    };
    Rendering::ShaderResource shader = Rendering::ShaderResource(
        {{SHADER_FILE("vertex.glsl"), Rendering::VertexShader, true},
        {SHADER_FILE("fragment.glsl"), Rendering::FragmentShader, true}});

    GLint modelMatrixUniform,
        viewMatrixUniform,
        projectionMatrixUniform;

    double lastCursorX = 0.0, lastCursorY = 0.0;
    bool orbiting = false, wireframeEnabled = true;

    std::vector<MeshObject> sceneObjects;

    // IMGUI
    bool imguiInitialized = false;
    // IMGUI

#pragma region Input Handlers
    static void HandleCursorPosition(GLFWwindow *window, double x, double y)
    {
        DemoScene *scene = nullptr;
        if (!Shared::GetWindowUserPointer(*window, scene))
            return;

        if (!scene->orbiting)
        {
            scene->orbiting = false;
            return;
        }

        const float sensitivity = 0.005f,
            yawDelta = static_cast<float>(x - scene->lastCursorX) * sensitivity,
            pitchDelta = static_cast<float>(scene->lastCursorY - y) * sensitivity;

        scene->camera.Orbit(yawDelta, pitchDelta);
        scene->lastCursorX = x;
        scene->lastCursorY = y;
    }

    static void HandleScroll(GLFWwindow *window, double, double yOffset)
    {
        DemoScene *scene = nullptr;
        if (!Shared::GetWindowUserPointer(*window, scene))
            return;

        constexpr float zoomSensitivity = 0.1f;
        scene->camera.Zoom(yOffset * zoomSensitivity);
    }
#pragma endregion

#pragma region Helper Funcs
    MeshObject *SpawnSceneObject(const Vector3f &position, const std::size_t meshType)
    {
        if (meshType >= meshes.size())
            return nullptr;

        sceneObjects.push_back({meshes[meshType].get(), {position}});
        return &sceneObjects[sceneObjects.size() - 1];
    }

    void RemoveSceneObject(const std::size_t objectIndex)
    {
        if (sceneObjects.size() > 0)
        sceneObjects.erase(sceneObjects.begin() + objectIndex);
    }

    bool &GetVSync() { return GetConfig().vsyncEnabled; }
    void SetVSync(const bool newValue)
    {
        auto &enabled = GetConfig().vsyncEnabled;
        if (newValue == enabled) return;
        enabled = newValue;
        glfwSwapInterval(enabled ? 1 : 0);
    }
#pragma endregion

    int InitResources()
    {
        using namespace Rendering;

        Rendering::MeshVertices vertices;
        Rendering::MeshIndices indices;
        if (Rendering::ParseMeshSource(MODEL_FILE("crystal.obj"), vertices, indices))
            meshes.push_back(std::make_shared<Rendering::MeshResource>(vertices, indices));

        std::vector<IResourceOwner *> resources{&shader};
        for (const auto &mesh : meshes)
            resources.push_back(mesh.get());

        if (!InitializeResources(resources))
        {
            LOG_ERROR("Failed to initialize resources.\n");
            return EXIT_FAILURE;
        }

        if (!shader.GetUniformLocation("modelMatrix", modelMatrixUniform) ||
            !shader.GetUniformLocation("viewMatrix", viewMatrixUniform) ||
            !shader.GetUniformLocation("projectionMatrix", projectionMatrixUniform))
        {
            LOG_ERROR("Transform or camera uniforms are missing from the shader program.\n");
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

    void BindInput()
    {
        using namespace Engine;
        
        GetInput().BindKey([this](const InputCode &, const KeyAction &action)
        {
            glfwSetWindowShouldClose(&GetWindow(), GLFW_TRUE);
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
            if (action != Pressed) return;
            SetVSync(!GetVSync());
        }, KeyCode::V);

        GetInput().BindKey([this](const InputCode &, const KeyAction &action)
        {
            if (action != Pressed) return;
                
            camera.ResetPosition();
            orbiting = false;
        }, KeyCode::R);

        GetInput().BindKey([this](const InputCode &, const KeyAction &action)
        {
            orbiting = static_cast<bool>(action);
            if (orbiting)
                glfwGetCursorPos(&GetWindow(), &lastCursorX, &lastCursorY);
        }, MouseKeyCode::Any);
    }

    int InitImgui()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGuiStyle &style = ImGui::GetStyle();
        style.Colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.18f, 0.18f, 0.18f, 1.f);
        style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.12f, 0.12f, 0.12f, 0.75f);

        if (!ImGui_ImplGlfw_InitForOpenGL(&GetWindow(), true))
        {
            LOG_ERROR("Failed to initialize ImGui GLFW backend.\n");
            ImGui::DestroyContext();
            return EXIT_FAILURE;
        }

        if (!ImGui_ImplOpenGL3_Init("#version 330"))
        {
            LOG_ERROR("Failed to initialize ImGui OpenGL backend.\n");
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();
            return EXIT_FAILURE;
        }

        imguiInitialized = true;
        return EXIT_SUCCESS;
    }

    int OnStart() override
    {
        using namespace Engine;

        if (InitResources() != EXIT_SUCCESS)
            return EXIT_FAILURE;

        BindInput();

        glfwSetCursorPosCallback(&GetWindow(), HandleCursorPosition);
        glfwSetScrollCallback(&GetWindow(), HandleScroll);

        camera.ResetPosition();

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

        return EXIT_SUCCESS;
    }

    void DrawImgui()
    {
        static Vector3f spawnPosition{0.f, 0.f, 0.f};
        static int spawnMeshType = 0;
        static bool enabled = true;

        static float targetFramerate = [](const double targetTimeStep)
        {
            if (!std::isfinite(targetTimeStep) || targetTimeStep <= 0.0)
                return 30.f;

            const double framerate = 1.0 / targetTimeStep;
            if (!std::isfinite(framerate) || framerate <= 0.0 ||
                framerate > std::numeric_limits<float>::max())
                return 30.f;

            return static_cast<float>(framerate);
        }(GetConfig().targetTimeStep.count());

        auto drawVector = [](const Vector3f &vector)
        {
            ImGui::TextColored({1.f, 0.3f, 0.3f, 1.f}, "X %.2f", vector.x);
            ImGui::SameLine();
            ImGui::TextColored({0.3f, 1.f, 0.3f, 1.f}, "Y %.2f", vector.y);
            ImGui::SameLine();
            ImGui::TextColored({0.3f, 0.5f, 1.f, 1.f}, "Z %.2f", vector.z);
        };

        const float framerate = 1.f / std::max(SMALL_NUMBER, GetSceneTime().deltaTime);

        if (!ImGui::Begin("Scene Context", &enabled))
        {
            ImGui::End();
            return;
        }

        ImGui::SeparatorText("Info");

        ImGui::Text("Framerate: %.1f FPS", framerate);
        const Vector3f &position = camera.GetTransform().GetPosition();
        const Vector3f direction = camera.GetTransform().GetForward();
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

        bool vsyncEnabled = GetVSync();
        if (ImGui::Checkbox("VSync (V)", &vsyncEnabled))
            SetVSync(vsyncEnabled);

        ImGui::BeginDisabled(vsyncEnabled);
        ImGui::SetNextItemWidth(100.f);
        ImGui::InputFloat("Target framerate", &targetFramerate, 1.f, 10.f, "%.1f");
        ImGui::EndDisabled();

        const bool invalidTargetFramerate =
            !vsyncEnabled && (!std::isfinite(targetFramerate) || targetFramerate <= 0.f);
        if (!invalidTargetFramerate)
        {
            GetConfig().targetTimeStep =
                vsyncEnabled
                ? std::chrono::duration<double>::zero()
                : std::chrono::duration<double>(1.0 / targetFramerate);
        }

        if (invalidTargetFramerate)
            ImGui::TextUnformatted("Target framerate must be finite and greater than zero.");

        ImGui::Spacing();
        ImGui::SeparatorText("Scene Objects");
        if (ImGui::BeginChild("SceneObjectList", ImVec2(0.f, 200.f), ImGuiChildFlags_Borders,
                ImGuiWindowFlags_AlwaysVerticalScrollbar))
        {
            for (std::size_t i = 0; i < sceneObjects.size(); ++i)
            {
                ImGui::PushID(static_cast<int>(i));
                MeshObject &object = sceneObjects[i];
                std::size_t meshIndex = 0;
                while (meshIndex < meshes.size() && object.mesh != meshes[meshIndex].get())
                    ++meshIndex;
                const Vector3f &objectPosition = object.transform.GetPosition();
                if (meshIndex < meshes.size())
                    ImGui::BulletText("%zu: Mesh %zu", i + 1, meshIndex + 1);
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
        {
            ImGui::TextUnformatted("No meshes available");
        }
        else
        {
            if (spawnMeshType < 0 || static_cast<std::size_t>(spawnMeshType) >= meshes.size())
                spawnMeshType = 0;

            const std::string preview = "Mesh " + std::to_string(spawnMeshType + 1);
            if (ImGui::BeginCombo("Mesh", preview.c_str()))
            {
                for (std::size_t meshIndex = 0; meshIndex < meshes.size(); ++meshIndex)
                {
                    const std::string label = "Mesh " + std::to_string(meshIndex + 1);
                    const bool selected = spawnMeshType == static_cast<int>(meshIndex);
                    if (ImGui::Selectable(label.c_str(), selected))
                        spawnMeshType = static_cast<int>(meshIndex);
                    if (selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }
        ImGui::InputFloat3("Position", glm::value_ptr(spawnPosition));
        ImGui::BeginDisabled(meshes.empty());
        if (ImGui::Button("Spawn Object"))
            SpawnSceneObject(spawnPosition, static_cast<std::size_t>(spawnMeshType));
        ImGui::EndDisabled();

        ImGui::End();
    }

    int OnDraw(const Engine::SceneTime &time) override
    {
        using namespace Engine;

        Vector3f moveVector{};
        GetInput().GetAxis3(KeyCode::D, KeyCode::A, KeyCode::Q, KeyCode::E,
            KeyCode::W, KeyCode::S, moveVector.x, moveVector.y, moveVector.z);
        camera.Move(moveVector, time.deltaTime);

        shader.Use();

        // Set the view and projection matrices based on the camera's current state.
        const Matrix4f viewMatrix = camera.GetViewMatrix();
        const Matrix4f projectionMatrix = camera.GetProjectionMatrix(ASPECT_RATIO(GetWindow()));
        glUniformMatrix4fv(viewMatrixUniform, 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(projectionMatrixUniform, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

        // Animate every other object in creation order.
        const float pulseScale = 1.f + glm::sin(time.elapsedTime) * 0.5f;
        for (auto &meshObject : sceneObjects)
        {
            if (!meshObject.animates)
                continue;

            meshObject.transform.RotateEuler({0.f, time.deltaTime, 0.f});
            meshObject.transform.Translate({0.f, glm::sin(time.elapsedTime) * time.deltaTime, 0.f});
            meshObject.transform.SetScale({pulseScale, pulseScale, pulseScale});
        }

        glPolygonMode(GL_FRONT_AND_BACK, wireframeEnabled ? GL_LINE : GL_FILL);
        
        for (auto &meshObject : sceneObjects)
        {
            const Matrix4f modelMatrix = meshObject.transform.GetTransformMatrix();
            glUniformMatrix4fv(modelMatrixUniform, 1, GL_FALSE, glm::value_ptr(modelMatrix));
            meshObject.mesh->Draw();
        }

        if (!imguiInitialized && InitImgui() != EXIT_SUCCESS)
            return EXIT_SUCCESS;

        IMGUI_DRAW_START
        DrawImgui();
        IMGUI_DRAW_END

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
    GLFWwindow *window = nullptr;
    int result = Shared::SetupWindow(window, 1200, 900, "Engine Window");

    if (result == EXIT_SUCCESS)
    {
        DemoScene scene;
        Engine::SceneConfig config{
            .targetTimeStep = 0.0333333s,
            .vsyncEnabled = true
        };
        result = scene.Start(*window, config);
    }

    if (window != nullptr)
        glfwDestroyWindow(window);

    glfwTerminate();

    return result;
}
