#pragma once

#include "InputCodes.h"
#include "Shared.h"

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif

#include <GLFW/glfw3.h>

#include <functional>

namespace Graphics
{
    struct WindowConfig
    {
        Int32 width = 800;
        Int32 height = 600;
        const char *title = "Window";
        bool vsyncEnabled = true;
    };

    struct InputCallbacks
    {
        std::function<void(Input::KeyCode, Input::KeyAction)> onKey;
        std::function<void(Input::MouseKeyCode, Input::KeyAction)> onMouseButton;
        std::function<void(double, double)> onCursorPosition;
        std::function<void(double, double)> onScroll;
    };

    // Wrapper class for GLFW window management and input handling.
    class Window
    {
        GLFWwindow *window = nullptr;
        InputCallbacks inputCallbacks;
        bool glfwInitialized = false;
        bool vsyncEnabled = true;

        bool InitializeWindow(const WindowConfig &config);

        static Window *GetHost(GLFWwindow *window) { return static_cast<Window *>(glfwGetWindowUserPointer(window)); }
        static void HandleKeyEvent(GLFWwindow *window, int key, int scancode, int action, int modifiers);
        static void HandleMouseButtonEvent(GLFWwindow *window, int button, int action, int modifiers);
        static void HandleCursorPosition(GLFWwindow *window, double x, double y);
        static void HandleScroll(GLFWwindow *window, double xOffset, double yOffset);
        static void HandleFramebufferSize(GLFWwindow *window, int width, int height);

    public:
        Window() = default;
        Window(const Window &) = delete;
        Window &operator=(const Window &) = delete;

        ~Window() { Shutdown(); }

        [[nodiscard]] bool IsVSyncEnabled() const { return vsyncEnabled; }
        [[nodiscard]] GLFWwindow &GetWindow() const { return *window; }
        [[nodiscard]] bool IsInitialized() const { return glfwInitialized && window; }
        [[nodiscard]] bool IsWindowFocused() const {return window && glfwGetWindowAttrib(window, GLFW_FOCUSED) == GLFW_TRUE;}
        [[nodiscard]] bool ShouldClose() const { return !window || glfwWindowShouldClose(window); }
        [[nodiscard]] float GetWindowAspectRatio() const;

        void GetCursorPosition(double &outX, double &outY) const { glfwGetCursorPos(&GetWindow(), &outX, &outY); }
        void PollEvents() const { glfwPollEvents();}

        bool Initialize(const WindowConfig &windowConfig = {}, const InputCallbacks &callbacks = {});
        void Shutdown() noexcept;
        void PresentFrame() const;
        void SetVSyncEnabled(bool enabled);
    };
}
