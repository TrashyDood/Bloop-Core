#pragma once

#ifdef LOGGING_ENABLED
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

class LogHandler
{
public:
    class Scope
    {
    public:
        Scope() { ++Depth(); }
        ~Scope() { --Depth(); }
        Scope(const Scope &) = delete;
        Scope &operator=(const Scope &) = delete;

        static std::uint8_t CurrentDepth()
        {
            const std::size_t depth = Depth();
            const std::size_t maxDepth = std::numeric_limits<std::uint8_t>::max();
            return static_cast<std::uint8_t>(depth > maxDepth ? maxDepth : depth);
        }

    private:
        static std::size_t &Depth()
        {
            static thread_local std::size_t depth = 0;
            return depth;
        }
    };

    enum LogLevel : std::uint8_t
    {
        Warning = 0,
        Info = 1,
        Error = 2
    };

    struct LogEntry
    {
        LogLevel level = LogLevel::Info;
        std::uint8_t priority = 0;
        std::uint8_t depth = 0;
        std::string message;
        std::chrono::system_clock::time_point timestamp{};
        std::string timestampText;
        std::thread::id threadId{};
    };

    template<typename... TArgs>
    void WriteLog(LogLevel level, std::uint8_t priority, std::uint8_t depth,
        std::string message, TArgs... args)
    {
        message = FormatMessage(message, args...);
        AddLogEntry(LogEntry{level, priority, depth, std::move(message)});
    }

    std::vector<LogEntry> GetLogEntries() const;
    void ClearLogEntries();

private:
    static constexpr std::size_t MaxLogEntries = 1000;
    mutable std::mutex mutex;
    std::deque<LogEntry> logEntries;

    void AddLogEntry(LogEntry entry);

    template<typename... TArgs>
    static std::string FormatMessage(const std::string &format, TArgs... args)
    {
        if constexpr (sizeof...(args) == 0)
            return format;
        else
        {
            const int messageSize = std::snprintf(nullptr, 0, format.c_str(), args...);
            if (messageSize < 0)
                throw std::runtime_error("Failed to format log entry.");

            std::vector<char> formattedMessage(static_cast<std::size_t>(messageSize) + 1);
            const int written =
                std::snprintf(formattedMessage.data(), formattedMessage.size(), format.c_str(), args...);
            if (written < 0)
                throw std::runtime_error("Failed to format log entry.");

            return std::string(formattedMessage.data(), static_cast<std::size_t>(written));
        }
    }
};

inline LogHandler &StaticLogHandler()
{
    static LogHandler logHandler;
    return logHandler;
}

#define LOG_SCOPE LogHandler::Scope _logScope_

#define LOG_ERROR(msg, ...)                                     LOG_ERROR_WITH_PRIORITY(0, msg, ##__VA_ARGS__)
#define LOG_WARNING(msg, ...)                                   LOG_WARNING_WITH_PRIORITY(0, msg, ##__VA_ARGS__)
#define LOG_INFO(msg, ...)                                      LOG_INFO_WITH_PRIORITY(0, msg, ##__VA_ARGS__)

#define LOG_ERROR_WITH_PRIORITY(priority, msg, ...)             StaticLogHandler().WriteLog(LogHandler::Error, priority, LogHandler::Scope::CurrentDepth(), msg, ##__VA_ARGS__)
#define LOG_WARNING_WITH_PRIORITY(priority, msg, ...)           StaticLogHandler().WriteLog(LogHandler::Warning, priority, LogHandler::Scope::CurrentDepth(), msg, ##__VA_ARGS__)
#define LOG_INFO_WITH_PRIORITY(priority, msg, ...)              StaticLogHandler().WriteLog(LogHandler::Info, priority, LogHandler::Scope::CurrentDepth(), msg, ##__VA_ARGS__)
#else
#define LOG_ERROR(msg, ...)
#define LOG_WARNING(msg, ...)
#define LOG_INFO(msg, ...)
#define LOG_SCOPE
#endif // LOGGING_ENABLED