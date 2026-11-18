#ifdef LOGGING_ENABLED
#include "LogHandler.h"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <sstream>

namespace
{
    const char *GetLogLevelName(LogHandler::LogLevel level)
    {
        switch (level)
        {
            case LogHandler::Warning: return "WARNING";
            case LogHandler::Info: return "INFO";
            case LogHandler::Error: return "ERROR";
            default: return "UNKNOWN";
        }
    }

    std::string FormatLogTimestamp(const std::chrono::system_clock::time_point timestamp)
    {
        const auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(timestamp);
        const auto milliseconds =
            std::chrono::duration_cast<std::chrono::milliseconds>(timestamp - seconds).count();
        const std::time_t time = std::chrono::system_clock::to_time_t(seconds);
        std::tm localTime{};
#ifdef _WIN32
        const bool hasLocalTime = localtime_s(&localTime, &time) == 0;
#else
        const bool hasLocalTime = localtime_r(&time, &localTime) != nullptr;
#endif

        char dateTime[32];
        if (!hasLocalTime || std::strftime(dateTime, sizeof(dateTime), "%Y-%m-%d %H:%M:%S", &localTime) == 0)
            return "timestamp unavailable";

        char formattedTimestamp[40];
        const int length = std::snprintf(
            formattedTimestamp, sizeof(formattedTimestamp), "%s.%03lld", dateTime,
            static_cast<long long>(milliseconds));
        if (length < 0 || static_cast<std::size_t>(length) >= sizeof(formattedTimestamp))
            return "timestamp unavailable";

        return formattedTimestamp;
    }
}

void LogHandler::AddLogEntry(LogEntry entry)
{
    std::lock_guard<std::mutex> lock(mutex);

    entry.timestamp = std::chrono::system_clock::now();
    entry.timestampText = FormatLogTimestamp(entry.timestamp);
    entry.threadId = std::this_thread::get_id();

    if (logEntries.size() >= MaxLogEntries)
        logEntries.pop_front();

    logEntries.push_back(std::move(entry));

    const LogEntry &storedEntry = logEntries.back();

    std::ostringstream threadIdStream;
    threadIdStream << storedEntry.threadId;

    const std::string threadIdText = threadIdStream.str();

    std::fprintf(stderr, "[%s]", storedEntry.timestampText.c_str());
    std::fprintf(stderr, " [%s] [thread %s] ", GetLogLevelName(storedEntry.level), threadIdText.c_str());

    for (std::uint8_t i = 0; i < storedEntry.depth; ++i)
        std::fputs("  ", stderr);
    
    std::fputs(storedEntry.message.c_str(), stderr);
}

std::vector<LogHandler::LogEntry> LogHandler::GetLogEntries() const
{
    std::lock_guard<std::mutex> lock(mutex);
    return {logEntries.begin(), logEntries.end()};
}

void LogHandler::ClearLogEntries()
{
    std::lock_guard<std::mutex> lock(mutex);
    logEntries.clear();
}
#endif // LOGGING_ENABLED