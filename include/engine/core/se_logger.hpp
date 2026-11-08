#pragma once

#include <string_view>

namespace se
{
    enum class LogLevel { Debug, Info, Warning, Error };

    // Borrowed by systems; the composition root keeps the sink alive through teardown.
    class Logger
    {
    public:
        Logger() = default;
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;
        Logger(Logger&&) = delete;
        Logger& operator=(Logger&&) = delete;

        void log(LogLevel level, std::string_view component, const char* format, ...) const noexcept;
    };
}
