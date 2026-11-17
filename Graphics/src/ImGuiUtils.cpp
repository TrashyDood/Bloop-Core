#ifdef IMGUI_ENABLED
#include "ImGuiUtils.h"

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
    }
}

void ImGuiUtils::Context::DrawLogWindow(LogHandler *logHandler)
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
    ImGui::Checkbox("Auto-scroll", &autoScroll);
    ImGui::SameLine();
    ImGui::Checkbox("Date", &showDate);
    ImGui::SameLine();
    ImGui::Checkbox("Time", &showTime);
    ImGui::SameLine();
    ImGui::Checkbox("Show depth", &showDepth);
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

        for (std::size_t i = 0; i < entries.size(); ++i)
        {
            const LogHandler::LogEntry &entry = entries[i];
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
            prefix += " ";

            ImGui::TextWrapped("%s%s", prefix.c_str(), entry.message.c_str());
            ImGui::PopStyleColor();

            ImGui::PopID();
        }

        if (autoScroll && wasAtBottom)
            ImGui::SetScrollHereY(1.f);
    }
    ImGui::EndChild();
    ImGui::End();
}
#endif // IMGUI_ENABLED