#pragma once

#ifdef IMGUI_ENABLED
#include "LogHandler.h"
#include "IOUtils.h"
#include "Shared.h"

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <chrono>
#include <functional>
#include <string>
#include <vector>

namespace ImGuiUtils
{
    struct SceneDiagnosticWindow
    {
        const std::vector<std::string> &shaderNames;
        const std::vector<std::string> &meshNames;
        const std::vector<std::string> &textureNames;

    private:
        std::chrono::steady_clock::time_point previousDrawTime{};
        UInt64 previousElapsedFrames = 0;
        bool hasPreviousDraw = false;

    public:
        SceneDiagnosticWindow(const std::vector<std::string> &shaderNames,
            const std::vector<std::string> &meshNames, const std::vector<std::string> &textureNames);

        void Draw(const float deltaTime, const float elapsedTime, const UInt64 elapsedFrames,
            const std::string &currentSceneName, const std::vector<IO::Path> &sceneFilePaths,
            const std::function<void(const std::string &)> &onSceneSelected);
    };

    struct LogWindow
    {
        enum class LogLevelFilter
        {
            All,
            Info,
            Warning,
            Error
        };

        LogLevelFilter logLevelFilter = LogLevelFilter::All;
        bool autoScroll = true,
            showDate = false,
            showTime = false,
            showDepth = false,
            showThread = true;

        void Draw(LogHandler *logHandler = nullptr);
    };

    class Context
    {
        ImGuiContext *imguiContext = nullptr;
        bool imguiGlfwInitialized = false,
            imguiOpenGLInitialized = false;

    public:
        Context() = default;
        ~Context() { Shutdown(); }
        Context(const Context &) = delete;
        Context &operator=(const Context &) = delete;

        bool Initialize(GLFWwindow &window);
        void Shutdown();
        static void Start();
        static void End();
    };
}
#endif // IMGUI_ENABLED