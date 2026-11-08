#include "engine/core/se_logger.hpp"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <unistd.h>

namespace se::log
{
    namespace
    {
        // Constant initialization avoids static initialization/destruction ordering hazards.
        // Only App's startup call writes these; callback threads only read them.
        constinit Level minimumLevel = Level::Debug;
        constinit int sink = -1;

        void emit(Level level, const char* label, std::string_view component,
                  const char* format, va_list args) noexcept
        {
            if (level < minimumLevel) return;

            // Bounded, per-call storage. Oversized messages are truncated, always with a newline.
            char message[16384];
            const int prefix = std::snprintf(message, sizeof(message), "[%s] [%.*s] ",
                label, static_cast<int>(component.size()), component.empty() ? "" : component.data());
            if (prefix < 0) return;
            const auto offset = std::min(static_cast<std::size_t>(prefix), sizeof(message) - 2);
            const int body = std::vsnprintf(message + offset, sizeof(message) - offset, format, args);
            const auto length = body < 0 ? offset : std::min(offset + static_cast<std::size_t>(body), sizeof(message) - 2);
            message[length] = '\n';
            // One syscall per message; no shared formatting buffer, mutex, or teardown dependency.
            (void)::write(sink < 0 ? STDERR_FILENO : sink, message, length + 1);
        }
    }

    void initialize(Level level) noexcept
    {
        // No reconfiguration, including during shutdown. Called only before other work.
        if (sink >= 0) return;
        minimumLevel = level;
        sink = STDERR_FILENO;
    }

    void debug(std::string_view component, const char* format, ...) noexcept
    {
        va_list args;
        va_start(args, format);
        emit(Level::Debug, "DEBUG", component, format, args);
        va_end(args);
    }

    void info(std::string_view component, const char* format, ...) noexcept
    {
        va_list args;
        va_start(args, format);
        emit(Level::Info, "INFO", component, format, args);
        va_end(args);
    }

    void warn(std::string_view component, const char* format, ...) noexcept
    {
        va_list args;
        va_start(args, format);
        emit(Level::Warning, "WARNING", component, format, args);
        va_end(args);
    }

    void error(std::string_view component, const char* format, ...) noexcept
    {
        va_list args;
        va_start(args, format);
        emit(Level::Error, "ERROR", component, format, args);
        va_end(args);
    }
}
