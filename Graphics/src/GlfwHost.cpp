#include "GlfwHost.h"
#include "GlfwInput.h"
#include "Graphics.h"

#include <cstdio>

#include <glad/gl.h>

namespace Graphics
{
    bool GlfwHost::InitializeWindow(const WindowConfig &config)
    {
        if (glfwInit() != GLFW_TRUE)
            return false;
        glfwInitialized = true;

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        window = glfwCreateWindow(config.width, config.height, config.title, nullptr, nullptr);
        if (window == nullptr)
        {
            LOG_ERROR("Failed to create GLFW window. Make sure that your graphics drivers are up to date and support OpenGL 3.3 or higher.\n");
            return false;
        }

        glfwMakeContextCurrent(window);
        if (gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress)) == 0)
        {
            LOG_ERROR("Failed to load OpenGL functions with GLAD. Make sure that your graphics drivers are up to date and support OpenGL 3.3 or higher.\n");
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

    void GlfwHost::Shutdown() noexcept
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

    bool GlfwHost::Initialize(const WindowConfig &windowConfig, const InputCallbacks &callbacks)
    {
        if (window != nullptr)
            return false;

        inputCallbacks = callbacks;
        vsyncEnabled = windowConfig.vsyncEnabled;
        if (!InitializeWindow(windowConfig))
        {
            Shutdown();
            return false;
        }

        SetVSyncEnabled(vsyncEnabled);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        return true;
    }

    void GlfwHost::PresentFrame()
    {
        CheckGLErrors();
        glfwSwapBuffers(window);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void GlfwHost::HandleKeyEvent(GLFWwindow *window, int key, int, int action, int)
    {
        GlfwHost *host = GetHost(window);
        if (host == nullptr)
            return;

        const auto translatedAction = ToSharedKeyAction(action);
        if (translatedAction && host->inputCallbacks.onKey)
            host->inputCallbacks.onKey(ToSharedKeyCode(key), *translatedAction);
    }

    void GlfwHost::HandleMouseButtonEvent(GLFWwindow *window, int button, int action, int)
    {
        GlfwHost *host = GetHost(window);
        if (host == nullptr)
            return;

        const auto translatedAction = ToSharedKeyAction(action);
        if (translatedAction && host->inputCallbacks.onMouseButton)
            host->inputCallbacks.onMouseButton(ToSharedMouseButton(button), *translatedAction);
    }

    void GlfwHost::HandleCursorPosition(GLFWwindow *window, double x, double y)
    {
        GlfwHost *host = GetHost(window);
        if (host != nullptr && host->inputCallbacks.onCursorPosition)
            host->inputCallbacks.onCursorPosition(x, y);
    }

    void GlfwHost::HandleScroll(GLFWwindow *window, double xOffset, double yOffset)
    {
        GlfwHost *host = GetHost(window);
        if (host != nullptr && host->inputCallbacks.onScroll)
            host->inputCallbacks.onScroll(xOffset, yOffset);
    }

    void GlfwHost::HandleFramebufferSize(GLFWwindow *, int width, int height)
    {
        glViewport(0, 0, width, height);
    }

    float GlfwHost::GetWindowAspectRatio() const
    {
        int width = 0, height = 0;
        glfwGetFramebufferSize(&GetWindow(), &width, &height);
        return static_cast<float>(glm::max(width, 1)) / static_cast<float>(glm::max(height, 1));
    }

    void GlfwHost::SetVSyncEnabled(const bool enabled)
    {
        vsyncEnabled = enabled;
        if (window != nullptr)
            glfwSwapInterval(vsyncEnabled ? 1 : 0);
    }
}
