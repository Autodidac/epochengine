module;

#ifndef NO_RETURN_RETRY_ONCE
#define NO_RETURN_RETRY_ONCE(call)                  \
    do {                                            \
        try {                                       \
            call;                                   \
        } catch (...) {                             \
            std::cerr << "[Retry] Retrying: " #call "\n"; \
            call;                                   \
        }                                           \
    } while (0)
#endif

#ifdef _WIN32
#include <consoleapi3.h>
#include "aframework.hpp"
#else
#include <unistd.h>
#include <stdio.h>
#endif

export module autilities;

import <iostream>;
import <type_traits>;
import <utility>;

export namespace almondnamespace::utilities
{
#ifdef _WIN32
    // MSVC may skip defining architecture macros when compiling module interfaces.
#if !defined(_X86_) && !defined(_AMD64_) && !defined(_ARM_) && !defined(_ARM64_)
#   if defined(_M_IX86)
#       define _X86_
#   elif defined(_M_AMD64) || defined(_M_X64)
#       define _AMD64_
#   elif defined(_M_ARM)
#       define _ARM_
#   elif defined(_M_ARM64)
#       define _ARM64_
#   endif
#endif

    [[nodiscard]] inline constexpr bool isConsoleApplication() noexcept
    {
        return GetConsoleWindow() != nullptr;
    }
#else
    [[nodiscard]] inline constexpr bool isConsoleApplication() noexcept
    {
        return isatty(fileno(stdout)) != 0;
    }
#endif

    template<typename Func>
    auto make_retry_once(Func&& f)
    {
        return [f = std::forward<Func>(f)](auto&&... args) -> decltype(auto)
        {
            try
            {
                return f(std::forward<decltype(args)>(args)...);
            }
            catch (...)
            {
                std::cerr << "[Retry] First attempt failed, retrying once...\n";
                return f(std::forward<decltype(args)>(args)...);
            }
        };
    }
}
