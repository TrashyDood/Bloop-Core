#pragma once

#include "InputCodes.h"

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif

#include <GLFW/glfw3.h>

#include <functional>

namespace Graphics
{
    struct WindowConfig
    {
        int width = 800;
        int height = 600;
        const char *title = "Engine Window";
        bool vsyncEnabled = true;
    };

    struct InputCallbacks
    {
        std::function<void(KeyCode, KeyAction)> onKey;
        std::function<void(MouseKeyCode, KeyAction)> onMouseButton;
        std::function<void(double, double)> onCursorPosition;
        std::function<void(double, double)> onScroll;
    };

    // Wrapper class for GLFW window management and input handling.
    class GlfwHost
    {
        GLFWwindow *window = nullptr;
        InputCallbacks inputCallbacks;
        bool glfwInitialized = false;
        bool vsyncEnabled = true;

        bool InitializeWindow(const WindowConfig &config);

        static GlfwHost *GetHost(GLFWwindow *window) { return static_cast<GlfwHost *>(glfwGetWindowUserPointer(window)); }
        static void HandleKeyEvent(GLFWwindow *window, int key, int scancode, int action, int modifiers);
        static void HandleMouseButtonEvent(GLFWwindow *window, int button, int action, int modifiers);
        static void HandleCursorPosition(GLFWwindow *window, double x, double y);
        static void HandleScroll(GLFWwindow *window, double xOffset, double yOffset);
        static void HandleFramebufferSize(GLFWwindow *window, int width, int height);

    public:
        GlfwHost() = default;
        GlfwHost(const GlfwHost &) = delete;
        GlfwHost &operator=(const GlfwHost &) = delete;

        ~GlfwHost() { Shutdown(); }

        [[nodiscard]] bool IsVSyncEnabled() const { return vsyncEnabled; }
        [[nodiscard]] GLFWwindow &GetWindow() const { return *window; }
        [[nodiscard]] bool IsInitialized() const { return glfwInitialized && window != nullptr; }
        [[nodiscard]] bool IsWindowFocused() const {return window != nullptr && glfwGetWindowAttrib(window, GLFW_FOCUSED) == GLFW_TRUE;}
        [[nodiscard]] bool ShouldClose() const { return window == nullptr || glfwWindowShouldClose(window); }
        [[nodiscard]] float GetWindowAspectRatio() const;

        void GetCursorPosition(double &outX, double &outY) const { glfwGetCursorPos(&GetWindow(), &outX, &outY); }
        void PollEvents() const { glfwPollEvents();}

        bool Initialize(const WindowConfig &windowConfig = {}, const InputCallbacks &callbacks = {});
        void Shutdown() noexcept;
        void PresentFrame();
        void SetVSyncEnabled(bool enabled);
    };
}
