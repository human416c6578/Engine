#pragma once

#include <string_view>

namespace se::log
{
    enum class Level { Debug, Info, Warning, Error };

    // App only: call once, before constructing systems or starting worker/driver work.
    // Afterwards configuration is read-only. Before init and through shutdown, stderr is valid.
    void initialize(Level minimumLevel) noexcept;

    void debug(std::string_view component, const char* format, ...) noexcept;
    void info(std::string_view component, const char* format, ...) noexcept;
    void warn(std::string_view component, const char* format, ...) noexcept;
    void error(std::string_view component, const char* format, ...) noexcept;
}
