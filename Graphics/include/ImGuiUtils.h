#pragma once

#ifdef IMGUI_ENABLED
#include "LogHandler.h"

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#define IMGUI_DRAW_START         \
    ImGui_ImplOpenGL3_NewFrame();\
    ImGui_ImplGlfw_NewFrame();   \
    ImGui::NewFrame();

#define IMGUI_DRAW_END                                     \
    ImGui::Render();                                       \
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

namespace ImGuiUtils
{
    class Context
    {
        enum class LogLevelFilter
        {
            All,
            Info,
            Warning,
            Error
        };

        ImGuiContext *imguiContext = nullptr;
        bool imguiGlfwInitialized = false;
        bool imguiOpenGLInitialized = false;
        bool autoScroll = true;
        bool showDate = false;
        bool showTime = false;
        bool showDepth = false;
        LogLevelFilter logLevelFilter = LogLevelFilter::All;

    public:
        Context() = default;
        ~Context() { Shutdown(); }
        Context(const Context &) = delete;
        Context &operator=(const Context &) = delete;

        bool Initialize(GLFWwindow &window);
        void Shutdown();
        void DrawLogWindow(LogHandler *logHandler = nullptr);
    };
}
#endif // IMGUI_ENABLED