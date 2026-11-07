#pragma once

#include "Shared/Shared.h"
#include "Shared/Transform.h"

struct SceneConfig
{
    std::chrono::duration<float> targetTimeStep = 0.016667s;
    bool enableVSync = true;
};

struct SceneTime
{
    float deltaTime = 0.0f;
    float elapsedTime = 0.0f;
};

class Scene
{
    bool isRunning = false;
    GLFWwindow *window;
    SceneConfig *config;

    int Start(GLFWwindow *window, SceneConfig *config);

protected:
    virtual int OnStart() = 0;
    virtual int OnUpdate(const SceneTime &time) = 0;
    virtual int OnDraw(const SceneTime &time) = 0;
    virtual void OnStop(const int exitCode) = 0;

public:
    [[nodiscard]] GLFWwindow &GetWindow() const { return *window; }
    [[nodiscard]] SceneConfig &GetConfig() const { return *config; }

    virtual ~Scene() = default;
    int Start(GLFWwindow &window, SceneConfig &config);
};