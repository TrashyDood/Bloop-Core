#include "Window.h"
#include "GlfwInput.h"
#include "Graphics.h"
#include "LogHandler.h"

#include <glad/gl.h>

namespace
{
    void LogGlfwError(const char *operation)
    {
        LOG_SCOPE;

        const char *description = nullptr;
        const int error = glfwGetError(&description);
        LOG_ERROR("%s failed (GLFW error 0x%X): %s\n",
            operation, static_cast<unsigned int>(error), description != nullptr ? description : "no description");
    }
}

namespace Graphics
{
    bool Window::InitializeWindow(const Config &config)
    {
        LOG_SCOPE;

        if (glfwInit() != GLFW_TRUE)
        {
            LogGlfwError("glfwInit");
            return false;
        }
        glfwInitialized = true;

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        window = glfwCreateWindow(config.width, config.height, config.title, nullptr, nullptr);
        if (window == nullptr)
        {
            LogGlfwError("glfwCreateWindow");
            return false;
        }

        glfwMakeContextCurrent(window);
        if (gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress)) == 0)
        {
            LOG_ERROR("Failed to load OpenGL functions with GLAD for the active context.\n");
            return false;
        }

        glfwSetWindowUserPointer(window, this);
        glfwSetKeyCallback(window, HandleKeyEvent);
        glfwSetMouseButtonCallback(window, HandleMouseButtonEvent);
        glfwSetCursorPosCallback(window, HandleCursorPosition);
        glfwSetScrollCallback(window, HandleScroll);
        glfwSetFramebufferSizeCallback(window, HandleFramebufferSize);

        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        return true;
    }

    void Window::Shutdown() noexcept
    {
        if (window != nullptr)
        {
            glfwSetWindowUserPointer(window, nullptr);
            glfwDestroyWindow(window);
            window = nullptr;
        }

        inputCallbacks = {};
        if (glfwInitialized)
        {
            glfwTerminate();
            glfwInitialized = false;
        }
    }

    bool Window::Initialize(const Window::Config &windowConfig, const Graphics::InputCallbacks &callbacks)
    {
        LOG_SCOPE;

        if (window != nullptr)
        {
            LOG_ERROR("Window initialization rejected: a window is already active.\n");
            return false;
        }

        inputCallbacks = callbacks;
        vsyncEnabled = windowConfig.vsyncEnabled;
        if (!InitializeWindow(windowConfig))
        {
            Shutdown();
            return false;
        }

        SetVSyncEnabled(vsyncEnabled);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        LOG_INFO("Initialized GLFW window (%d x %d, VSync %s).\n",
            windowConfig.width, windowConfig.height, vsyncEnabled ? "on" : "off");
        return true;
    }

    void Window::PresentFrame() const
    {
        LOG_SCOPE;

        CheckGLErrors();
        glfwSwapBuffers(window);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void Window::HandleKeyEvent(GLFWwindow *window, int key, int, int action, int)
    {
        Window *host = GetHost(window);
        if (host == nullptr)
            return;

        const auto translatedAction = ToSharedKeyAction(action);
        if (translatedAction && host->inputCallbacks.onKey)
            host->inputCallbacks.onKey(ToSharedKeyCode(key), *translatedAction);
    }

    void Window::HandleMouseButtonEvent(GLFWwindow *window, int button, int action, int)
    {
        Window *host = GetHost(window);
        if (host == nullptr)
            return;

        const auto translatedAction = ToSharedKeyAction(action);
        if (translatedAction && host->inputCallbacks.onMouseButton)
            host->inputCallbacks.onMouseButton(ToSharedMouseButton(button), *translatedAction);
    }

    void Window::HandleCursorPosition(GLFWwindow *window, double x, double y)
    {
        Window *host = GetHost(window);
        if (host != nullptr && host->inputCallbacks.onCursorPosition)
            host->inputCallbacks.onCursorPosition(x, y);
    }

    void Window::HandleScroll(GLFWwindow *window, double xOffset, double yOffset)
    {
        Window *host = GetHost(window);
        if (host != nullptr && host->inputCallbacks.onScroll)
            host->inputCallbacks.onScroll(xOffset, yOffset);
    }

    void Window::HandleFramebufferSize(GLFWwindow *, int width, int height)
    {
        glViewport(0, 0, width, height);
    }

    float Window::GetWindowAspectRatio() const
    {
        int width = 0, height = 0;
        glfwGetFramebufferSize(&GetWindow(), &width, &height);
        return static_cast<float>(glm::max(width, 1)) / static_cast<float>(glm::max(height, 1));
    }

    bool Window::GetFrameBufferSize(Int32 &outWidth, Int32 &outHeight)
    {
        glfwGetFramebufferSize(&GetWindow(), &outWidth, &outHeight);
        return outWidth > 0 && outHeight > 0;
    }

    bool Window::RenderViewport(const UInt16 index, const UInt16 count, const ViewportCallback &callback, const ViewportSplitRule &splitRule)
    {
        Int32 bufferWidth = 0, bufferHeight = 0;
        if (!GetFrameBufferSize(bufferWidth, bufferHeight))
            return false;

        const Box2i viewportRect = splitRule(static_cast<UInt32>(bufferWidth),
            static_cast<UInt32>(bufferHeight), index, count);

        if (viewportRect.Width() <= 0 || viewportRect.Height() <= 0)
            return false;

        glViewport(viewportRect.position.x, viewportRect.position.y,
            viewportRect.dimensions.x, viewportRect.dimensions.y);

        callback({index, count, viewportRect, static_cast<float>(viewportRect.dimensions.x) / viewportRect.dimensions.y});

        return true;
    }

    bool Window::RenderViewports(const UInt16 count, const ViewportCallback &callback, const ViewportSplitRule &splitRule)
    {
        if (count < 1 || !splitRule || !callback)
            return false;

        for (UInt16 i = 0; i < count; ++i)
            RenderViewport(i, count, callback, splitRule);

        return true;
    }

    void Window::SetVSyncEnabled(const bool enabled)
    {
        vsyncEnabled = enabled;
        if (window != nullptr)
            glfwSwapInterval(vsyncEnabled ? 1 : 0);
    }
}