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
    struct InputCallbacks
    {
        std::function<void(Input::KeyCode, Input::KeyAction)> onKey{};
        std::function<void(Input::MouseKeyCode, Input::KeyAction)> onMouseButton{};
        std::function<void(double, double)> onCursorPosition{};
        std::function<void(double, double)> onScroll{};
    };

    // Wrapper class for GLFW window management and input handling.
    class Window
    {
    public:
        struct Viewport
        {
            const UInt16 index;
            const UInt16 count;
            const Box2i rect;
            const float aspectRatio;
        };

        struct Config
        {
            Int32 width = 800;
            Int32 height = 600;
            const char *title = "Window";
            bool vsyncEnabled = true;

            Config() = default;
        };

    private:
        using ViewportCallback = std::function<void(const Viewport&)>;
        using ViewportSplitRule = std::function<Box2i(UInt32, UInt32, UInt16, UInt16)>;

        GLFWwindow *window = nullptr;
        InputCallbacks inputCallbacks;
        bool glfwInitialized = false,
            vsyncEnabled = true;

        static constexpr auto DefaultSplitRule = [](UInt32 width, UInt32 height, UInt16 index, UInt16 viewportCount)
        {
            if (viewportCount == 0 || index >= viewportCount)
                return Box2i{};

            UInt16 columns = 1;
            while (static_cast<UInt32>(columns) * columns < viewportCount)
                ++columns;

            const UInt16 rows = static_cast<UInt16>((viewportCount + columns - 1) / columns);
            const UInt16 column = index % columns;
            const UInt16 rowFromTop = index / columns;
            const UInt16 row = rows - 1 - rowFromTop;
            const UInt16 itemsInRow = rowFromTop == rows - 1
                ? static_cast<UInt16>(viewportCount - rowFromTop * columns)
                : columns;
            const UInt16 horizontalOffset = columns - itemsInRow;

            const UInt64 gridWidth = static_cast<UInt64>(columns) * 2;
            const UInt64 leftUnits = static_cast<UInt64>(column) * 2 + horizontalOffset;
            const UInt64 rightUnits = static_cast<UInt64>(column + 1) * 2 + horizontalOffset;
            const Int32 left = static_cast<Int32>(static_cast<UInt64>(width) * leftUnits / gridWidth);
            const Int32 right = static_cast<Int32>(static_cast<UInt64>(width) * rightUnits / gridWidth);
            const Int32 bottom = static_cast<Int32>(static_cast<UInt64>(height) * row / rows);
            const Int32 top = static_cast<Int32>(static_cast<UInt64>(height) * (row + 1) / rows);

            Box2i rect;
            rect.position = Vector2i{left, bottom};
            rect.dimensions = Vector2i{right - left, top - bottom};
            return rect;
        };

        bool InitializeWindow(const Config &config);

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
        [[nodiscard]] bool IsWindowFocused() const { return window && glfwGetWindowAttrib(window, GLFW_FOCUSED) == GLFW_TRUE; }
        [[nodiscard]] bool ShouldClose() const { return !window || glfwWindowShouldClose(window); }
        [[nodiscard]] float GetWindowAspectRatio() const;

        bool GetFrameBufferSize(Int32 &outWidth, Int32 &outHeight);

        bool RenderViewport(const UInt16 index, const UInt16 count, const ViewportCallback &callback, const ViewportSplitRule &splitRule = DefaultSplitRule);
        bool RenderViewports(const UInt16 count, const ViewportCallback &callback, const ViewportSplitRule &viewportRule = DefaultSplitRule);

        void GetCursorPosition(double &outX, double &outY) const { glfwGetCursorPos(&GetWindow(), &outX, &outY); }
        void PollEvents() const { glfwPollEvents();}

        bool Initialize(const Config &windowConfig, const InputCallbacks &callbacks = {});
        void Shutdown() noexcept;
        void PresentFrame() const;
        void SetVSyncEnabled(bool enabled);
    };
}
