#include "engine/core/se_logger.hpp"

#include <cstdarg>
#include <cstdio>

namespace se
{
    void Logger::log(LogLevel level, std::string_view component, const char* format, ...) const noexcept
    {
        const char* label = "DEBUG";
        switch (level)
        {
        case LogLevel::Debug: label = "DEBUG"; break;
        case LogLevel::Info: label = "INFO"; break;
        case LogLevel::Warning: label = "WARNING"; break;
        case LogLevel::Error: label = "ERROR"; break;
        }
        std::fprintf(stderr, "[%s] [%.*s] ", label, static_cast<int>(component.size()), component.data());
        va_list args;
        va_start(args, format);
        std::vfprintf(stderr, format, args);
        va_end(args);
        std::fputc('\n', stderr);
    }
}
