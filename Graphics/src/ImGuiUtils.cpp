#ifdef IMGUI_ENABLED
#include "ImGuiUtils.h"

#include <algorithm>
#include <cmath>
#include <sstream>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <psapi.h>
#endif

bool ImGuiUtils::Context::Initialize(GLFWwindow &window)
{
    LOG_SCOPE;

    if (imguiContext != nullptr && imguiGlfwInitialized && imguiOpenGLInitialized)
        return true;

    if (imguiContext != nullptr || imguiGlfwInitialized || imguiOpenGLInitialized)
        Shutdown();

    IMGUI_CHECKVERSION();
    imguiContext = ImGui::CreateContext();
    if (imguiContext == nullptr)
    {
        LOG_ERROR("Failed to create ImGui context.\n");
        return false;
    }
    ImGui::SetCurrentContext(imguiContext);

    ImGui::StyleColorsDark();
    ImGuiStyle &style = ImGui::GetStyle();
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.18f, 0.18f, 0.18f, 1.f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.12f, 0.12f, 0.12f, 0.75f);

    if (!ImGui_ImplGlfw_InitForOpenGL(&window, true))
    {
        LOG_ERROR("Failed to initialize ImGui GLFW backend.\n");
        Shutdown();
        return false;
    }
    imguiGlfwInitialized = true;

    if (!ImGui_ImplOpenGL3_Init("#version 330"))
    {
        LOG_ERROR("Failed to initialize ImGui OpenGL backend.\n");
        Shutdown();
        return false;
    }
    imguiOpenGLInitialized = true;

    LOG_INFO("ImGui initialized.\n");
    return true;
}

void ImGuiUtils::Context::Shutdown()
{
    LOG_SCOPE;

    if (imguiContext == nullptr && !imguiGlfwInitialized && !imguiOpenGLInitialized)
        return;

    LOG_INFO("Shutting down ImGui.\n");
    if (imguiContext != nullptr)
        ImGui::SetCurrentContext(imguiContext);

    if (imguiOpenGLInitialized)
    {
        if (ImGui::GetCurrentContext() != nullptr &&
            ImGui::GetIO().BackendRendererUserData != nullptr)
            ImGui_ImplOpenGL3_Shutdown();
        else
            LOG_ERROR("ImGui OpenGL backend state is missing during shutdown.\n");
        imguiOpenGLInitialized = false;
    }

    if (imguiGlfwInitialized)
    {
        if (ImGui::GetCurrentContext() != nullptr &&
            ImGui::GetIO().BackendPlatformUserData != nullptr)
            ImGui_ImplGlfw_Shutdown();
        else
            LOG_ERROR("ImGui GLFW backend state is missing during shutdown.\n");
        imguiGlfwInitialized = false;
    }

    if (imguiContext != nullptr)
    {
        ImGui::DestroyContext(imguiContext);
        imguiContext = nullptr;
        ImGui::SetCurrentContext(nullptr);
    }
}

void ImGuiUtils::Context::Start()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiUtils::Context::End()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

ImGuiUtils::SceneDiagnosticWindow::SceneDiagnosticWindow(
    const std::vector<std::string> &shaderNames, const std::vector<std::string> &meshNames,
    const std::vector<std::string> &textureNames)
    : shaderNames(shaderNames), meshNames(meshNames), textureNames(textureNames) { }

void ImGuiUtils::SceneDiagnosticWindow::Draw(
    const float deltaTime, const float elapsedTime, const UInt64 elapsedFrames,
    const std::string &currentSceneName, const std::vector<IO::Path> &sceneFilePaths,
    const std::function<void(const std::string &)> &onSceneSelected,
    const std::function<void()> &onPlayerSpawnRequested,
    const std::function<void()> &onPlayerRemovalRequested)
{
    LOG_SCOPE;

    const auto now = std::chrono::steady_clock::now();
    if (hasPreviousDraw && elapsedFrames < previousElapsedFrames)
        hasPreviousDraw = false;
    const double drawInterval = hasPreviousDraw
        ? std::chrono::duration<double>(now - previousDrawTime).count()
        : 0.0;
    previousDrawTime = now;
    previousElapsedFrames = elapsedFrames;
    hasPreviousDraw = true;

    if (!ImGui::Begin("Context"))
    {
        ImGui::End();
        return;
    }

    ImGui::SeparatorText("Scene");
    ImGui::Text("Current scene: %s", currentSceneName.c_str());
    if (ImGui::BeginCombo("Load scene", currentSceneName.c_str()))
    {
        for (const auto &scenePath : sceneFilePaths)
        {
            const std::string sceneName = scenePath.relativePath.stem().string();
            const bool isSelected = sceneName == currentSceneName;
            if (ImGui::Selectable(sceneName.c_str(), isSelected) &&
                !isSelected && onSceneSelected)
                onSceneSelected(sceneName);
            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    if (ImGui::Button("Spawn player") && onPlayerSpawnRequested)
        onPlayerSpawnRequested();
    ImGui::SameLine();
    if (ImGui::Button("Remove player") && onPlayerRemovalRequested)
        onPlayerRemovalRequested();

    ImGui::SeparatorText("Time");
    ImGui::Text("Tick framerate: %.1f FPS (%.2f ms)",
        1.f / std::max(SMALL_NUMBER, deltaTime), deltaTime * 1000.f);
    const float drawFramerate = drawInterval > 0.0
        ? static_cast<float>(1.0 / drawInterval)
        : 0.f;
    ImGui::Text("Last draw interval: %.1f FPS (%.2f ms)", drawFramerate, drawInterval * 1000.0);
    ImGui::Text("Elapsed frames: %llu", static_cast<unsigned long long>(elapsedFrames));
    ImGui::Text("Time elapsed: %02d:%05.2f", static_cast<int>(elapsedTime / 60.f),
        std::fmod(elapsedTime, 60.f));

#ifdef _WIN32
    ImGui::SeparatorText("Process Memory");

    PROCESS_MEMORY_COUNTERS_EX memoryCounters{};
    memoryCounters.cb = sizeof(memoryCounters);
    static bool memoryQueryFailureLogged = false;
    if (GetProcessMemoryInfo(GetCurrentProcess(),
            reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&memoryCounters), sizeof(memoryCounters)))
    {
        constexpr double bytesPerMebibyte = 1024.0 * 1024.0;
        ImGui::Text("Working set: %.1f MiB", memoryCounters.WorkingSetSize / bytesPerMebibyte);
        ImGui::Text("Private usage: %.1f MiB", memoryCounters.PrivateUsage / bytesPerMebibyte);
        memoryQueryFailureLogged = false;
    }
    else
    {
        const DWORD error = GetLastError();
        if (!memoryQueryFailureLogged)
        {
            LOG_WARNING("Failed to query current process memory usage (error %lu).\n",
                static_cast<unsigned long>(error));
            memoryQueryFailureLogged = true;
        }
        ImGui::TextDisabled("Process memory usage unavailable.");
    }
#else
    ImGui::SeparatorText("Process Memory");
    ImGui::TextDisabled("Process memory metrics are available on Windows only.");
#endif

    ImGui::SeparatorText("Loaded Resources");
    const auto drawResourceList = [](const char *label, const std::vector<std::string> &names)
    {
        if (ImGui::TreeNode(label, "%s (%zu)", label, names.size()))
        {
            if (names.empty())
                ImGui::TextDisabled("None");
            else
                for (const std::string &name : names)
                    ImGui::BulletText("%s", name.c_str());

            ImGui::TreePop();
        }
    };

    drawResourceList("Shaders", shaderNames);
    drawResourceList("Meshes", meshNames);
    drawResourceList("Textures", textureNames);
    ImGui::End();
}

void ImGuiUtils::LogWindow::Draw(LogHandler *logHandler)
{
    if (!logHandler) logHandler = &StaticLogHandler();

    if (!ImGui::Begin("Log"))
    {
        ImGui::End();
        return;
    }

    if (ImGui::Button("Clear"))
        logHandler->ClearLogEntries();
    ImGui::SameLine();
    const char *filterLabel = "All";
    switch (logLevelFilter)
    {
        case LogLevelFilter::Info: filterLabel = "Info"; break;
        case LogLevelFilter::Warning: filterLabel = "Warnings"; break;
        case LogLevelFilter::Error: filterLabel = "Errors"; break;
        case LogLevelFilter::All: break;
    }
    ImGui::SetNextItemWidth(110.f);
    if (ImGui::BeginCombo("Filter", filterLabel))
    {
        if (ImGui::Selectable("All", logLevelFilter == LogLevelFilter::All))
            logLevelFilter = LogLevelFilter::All;
        if (ImGui::Selectable("Info", logLevelFilter == LogLevelFilter::Info))
            logLevelFilter = LogLevelFilter::Info;
        if (ImGui::Selectable("Warnings", logLevelFilter == LogLevelFilter::Warning))
            logLevelFilter = LogLevelFilter::Warning;
        if (ImGui::Selectable("Errors", logLevelFilter == LogLevelFilter::Error))
            logLevelFilter = LogLevelFilter::Error;
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &autoScroll);
    ImGui::SameLine();
    ImGui::Checkbox("Date", &showDate);
    ImGui::SameLine();
    ImGui::Checkbox("Time", &showTime);
    ImGui::SameLine();
    ImGui::Checkbox("Depth", &showDepth);
    ImGui::SameLine();
    ImGui::Checkbox("Thread", &showThread);
    ImGui::Separator();

    const auto getTimestampText = [&](const std::string &timestamp)
    {
        if (!showDate && !showTime)
            return std::string{};

        const std::size_t separator = timestamp.find(' ');
        if (separator == std::string::npos || (showDate && showTime))
            return timestamp;

        return showDate ? timestamp.substr(0, separator) : timestamp.substr(separator + 1);
    };

    if (ImGui::BeginChild("LogEntries", ImVec2(0.f, 0.f), ImGuiChildFlags_Borders))
    {
        const bool wasAtBottom = ImGui::GetScrollY() >= ImGui::GetScrollMaxY();
        const std::vector<LogHandler::LogEntry> entries = logHandler->GetLogEntries();
        if (entries.empty())
            ImGui::TextDisabled("No log entries.");

        std::size_t visibleEntryCount = 0;
        for (std::size_t i = 0; i < entries.size(); ++i)
        {
            const LogHandler::LogEntry &entry = entries[i];
            if ((logLevelFilter == LogLevelFilter::Info && entry.level != LogHandler::Info) ||
                (logLevelFilter == LogLevelFilter::Warning && entry.level != LogHandler::Warning) ||
                (logLevelFilter == LogLevelFilter::Error && entry.level != LogHandler::Error))
                continue;

            ++visibleEntryCount;
            ImGui::PushID(static_cast<int>(i));
            if (showDepth)
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                    static_cast<float>(entry.depth) * ImGui::GetStyle().IndentSpacing);

            const char *levelName = "UNKNOWN";
            ImVec4 levelColor{0.8f, 0.8f, 0.8f, 1.f};
            switch (entry.level)
            {
                case LogHandler::Warning:
                    levelName = "WARNING";
                    levelColor = {1.f, 0.75f, 0.2f, 1.f};
                    break;
                case LogHandler::Info:
                    levelName = "INFO";
                    levelColor = {0.6f, 0.8f, 1.f, 1.f};
                    break;
                case LogHandler::Error:
                    levelName = "ERROR";
                    levelColor = {1.f, 0.35f, 0.35f, 1.f};
                    break;
            }
            ImGui::PushStyleColor(ImGuiCol_Text, levelColor);
            const std::string timestampText = getTimestampText(entry.timestampText);
            std::string prefix = "[" + std::to_string(i + 1);
            if (showDepth)
                prefix += "-" + std::to_string(static_cast<unsigned int>(entry.depth));
            prefix += "] ";
            if (!timestampText.empty())
                prefix += "[" + timestampText + "] ";
            prefix += "[" + std::string(levelName) + "]";
            if (entry.priority >= 1)
                prefix += " [P" + std::to_string(static_cast<unsigned int>(entry.priority)) + "]";
            if (showThread)
            {
                std::ostringstream threadId;
                threadId << entry.threadId;
                prefix += " [thread " + threadId.str() + "]";
            }
            prefix += " ";

            ImGui::TextWrapped("%s%s", prefix.c_str(), entry.message.c_str());
            ImGui::PopStyleColor();

            ImGui::PopID();
        }

        if (!entries.empty() && visibleEntryCount == 0)
            ImGui::TextDisabled("No log entries match this filter.");

        if (autoScroll && wasAtBottom)
            ImGui::SetScrollHereY(1.f);
    }
    ImGui::EndChild();
    ImGui::End();
}
#endif // IMGUI_ENABLED