#pragma once

#include "spdlog/spdlog.h"

namespace Apex
{
    namespace Color
    {
        static constexpr auto magenta = "\033[35m";
        static constexpr auto green = "\033[32m";
        static constexpr auto red = "\033[31m";
        static constexpr auto reset = "\033[0m";
    }  // namespace Color

    class Log
    {
    public:
        template <typename... Args>
        static void Info(const fmt::format_string<Args...>& fmt, Args&&... args)
        {
            spdlog::info(fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void Warn(const fmt::format_string<Args...>& fmt, Args&&... args)
        {
            spdlog::warn(fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void Error(const fmt::format_string<Args...>& fmt, Args&&... args)
        {
            spdlog::error(fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void Critical(const fmt::format_string<Args...>& fmt, Args&&... args)
        {
            spdlog::critical(fmt, std::forward<Args>(args)...);
        }
    };
}  // namespace Apex
