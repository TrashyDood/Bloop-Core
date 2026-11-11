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

#include <cstdlib>
#include <array>
#include <cmath>

#define IMGUI_FRAME_START                                                                                              \
    ImGui_ImplOpenGL3_NewFrame();                                                                                      \
    ImGui_ImplGlfw_NewFrame();                                                                                         \
    ImGui::NewFrame();

#define IMGUI_FRAME_END                                                                                                \
    ImGui::Render();                                                                                                   \
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

struct MeshObject { Rendering::Mesh &mesh; Shared::Transform transform; };
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

class DemoScene : public Scene
{
    Rendering::Shader shader;
    Rendering::FlyingCamera camera;
    Rendering::Mesh cubeMesh{CubeGeometry.vertices, CubeGeometry.indices};
    Rendering::Mesh pyramidMesh{PyramidGeometry.vertices, PyramidGeometry.indices};

    GLint modelMatrixUniform,
        viewMatrixUniform,
        projectionMatrixUniform;

    double lastCursorX = 0.0, lastCursorY = 0.0;
    bool orbiting = false, wireframeEnabled = true;

    std::array<MeshObject, 4> sceneObjects{{
        {cubeMesh, {{4.f, 0.f, 0.f}}},
        {cubeMesh, {{2.f, 0.f, 0.f}}},
        {pyramidMesh, {{0.f, 0.f, 0.f}}},
        {pyramidMesh, {{-2.f, 0.f, 0.f}}}
    }};

    // IMGUI
    bool imguiInitialized = false,
        imguiWindowEnabled = true;
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

    int InitResources()
    {
        using namespace Rendering;

        shader = Shader({{"vertex.glsl", VertexShader, true},
                         {"fragment.glsl", FragmentShader, true}});

        if (!InitializeResources({&shader, &cubeMesh, &pyramidMesh}))
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
            GetConfig().enableVSync = !GetConfig().enableVSync;
            glfwSwapInterval(GetConfig().enableVSync ? 1 : 0);
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
        static float targetFramerate = 1.f / GetConfig().targetTimeStep.count();
        static bool invalidTargetFramerate = false;
        const float framerate = 1.f / std::max(0.0000000001f, GetSceneTime().deltaTime);

        ImGui::Begin("Scene Context", &imguiWindowEnabled);
        ImGui::SeparatorText("Info");

        ImGui::Text("Framerate: %.1f FPS", framerate);
        const Vector3f &position = camera.GetTransform().GetPosition();
        const Vector3f direction = camera.GetTransform().GetForward();
        ImGui::TextUnformatted("Camera position:");
        ImGui::SameLine();
        ImGui::TextColored({1.f, 0.3f, 0.3f, 1.f}, "X %.2f", position.x);
        ImGui::SameLine();
        ImGui::TextColored({0.3f, 1.f, 0.3f, 1.f}, "Y %.2f", position.y);
        ImGui::SameLine();
        ImGui::TextColored({0.3f, 0.5f, 1.f, 1.f}, "Z %.2f", position.z);
        ImGui::TextUnformatted("Camera facing:");
        ImGui::SameLine();
        ImGui::TextColored({1.f, 0.3f, 0.3f, 1.f}, "X %.2f", direction.x);
        ImGui::SameLine();
        ImGui::TextColored({0.3f, 1.f, 0.3f, 1.f}, "Y %.2f", direction.y);
        ImGui::SameLine();
        ImGui::TextColored({0.3f, 0.5f, 1.f, 1.f}, "Z %.2f", direction.z);
        ImGui::Text("Camera distance: %.2f", camera.GetOrbitDistance());

        if (ImGui::Button("Reset camera (R)"))
        {
            camera.ResetPosition();
            orbiting = false;
        }

        ImGui::Spacing();
        ImGui::SeparatorText("Settings");

        bool isOrthographic = camera.GetIsOrthographic();
        if (ImGui::Checkbox("Orthographic (G)", &isOrthographic))
            camera.SetIsOrthographic(isOrthographic);
        ImGui::Checkbox("Wireframe (F)", &wireframeEnabled);

        if (ImGui::Checkbox("VSync (V)", &GetConfig().enableVSync))
        {
            glfwSwapInterval(GetConfig().enableVSync ? 1 : 0);
            if (GetConfig().enableVSync)
                GetConfig().targetTimeStep = std::chrono::duration<double>::zero();
        }

        ImGui::BeginDisabled(GetConfig().enableVSync);
        ImGui::SetNextItemWidth(100.f);
        ImGui::InputFloat("Target framerate", &targetFramerate, 1.f, 10.f, "%.1f");
        ImGui::EndDisabled();

        invalidTargetFramerate =
            !GetConfig().enableVSync && (!std::isfinite(targetFramerate) || targetFramerate <= 0.f);
        if (!invalidTargetFramerate)
        {
            GetConfig().targetTimeStep =
                GetConfig().enableVSync
                ? std::chrono::duration<double>::zero()
                : std::chrono::duration<double>(1.0 / targetFramerate);
        }

        if (invalidTargetFramerate)
            ImGui::TextUnformatted("Target framerate must be greater than zero.");
        ImGui::End();
    }

    int OnDraw(const SceneTime &time) override
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

        const float pulseScale = 1.f + glm::sin(time.elapsedTime) * 0.5f;

        sceneObjects[0].transform.RotateEuler({0.f, 1.f * time.deltaTime, 0.f});
        sceneObjects[0].transform.Translate({0.f, glm::sin(time.elapsedTime) * time.deltaTime, 0.f});
        sceneObjects[0].transform.SetScale({pulseScale, pulseScale, pulseScale});

        sceneObjects[2].transform.RotateEuler({0.f, 1.f * time.deltaTime, 0.f});
        sceneObjects[2].transform.Translate({0.f, glm::sin(time.elapsedTime) * time.deltaTime, 0.f});
        sceneObjects[2].transform.SetScale({pulseScale, pulseScale, pulseScale});

        glPolygonMode(GL_FRONT_AND_BACK, wireframeEnabled ? GL_LINE : GL_FILL);

        for (auto& meshObject : sceneObjects)
        {
            const Matrix4f modelMatrix = meshObject.transform.GetTransformMatrix();
            glUniformMatrix4fv(modelMatrixUniform, 1, GL_FALSE, glm::value_ptr(modelMatrix));
            meshObject.mesh.Draw();
        }

        if (!imguiInitialized && InitImgui() != EXIT_SUCCESS)
            return EXIT_SUCCESS;

        IMGUI_FRAME_START
        DrawImgui();
        IMGUI_FRAME_END

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
        SceneConfig config{
            .targetTimeStep = 0.0333333s,
            .enableVSync = false
        };
        result = scene.Start(*window, config);
    }

    if (window != nullptr)
        glfwDestroyWindow(window);

    glfwTerminate();

    return result;
}
