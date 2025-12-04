/**************************************************************
 *   █████╗ ██╗     ███╗   ███╗   ███╗   ██╗    ██╗██████╗    *
 *  ██╔══██╗██║     ████╗ ████║ ██╔═══██╗████╗  ██║██╔══██╗   *
 *  ███████║██║     ██╔████╔██║ ██║   ██║██╔██╗ ██║██║  ██║   *
 *  ██╔══██║██║     ██║╚██╔╝██║ ██║   ██║██║╚██╗██║██║  ██║   *
 *  ██║  ██║███████╗██║ ╚═╝ ██║ ╚██████╔╝██║ ╚████║██████╔╝   *
 *  ╚═╝  ╚═╝╚══════╝╚═╝     ╚═╝  ╚═════╝ ╚═╝  ╚═══╝╚═════╝    *
 *                                                            *
 *   This file is part of the Almond Project.                 *
 *   AlmondShell - Modular C++ Framework                      *
 *                                                            *
 *   SPDX-License-Identifier: LicenseRef-MIT-NoSell           *
 *                                                            *
 *   Provided "AS IS", without warranty of any kind.          *
 *   Use permitted for Non-Commercial Purposes ONLY,          *
 *   without prior commercial licensing agreement.            *
 *                                                            *
 *   Redistribution Allowed with This Notice and              *
 *   LICENSE file. No obligation to disclose modifications.   *
 *                                                            *
 *   See LICENSE file for full terms.                         *
 *                                                            *
 **************************************************************/
module;

#include <iostream>
#include <type_traits>
#include <utility>

#ifdef _WIN32
#include <consoleapi3.h>
#else
#include <stdio.h>
#include <unistd.h>
#endif

export module almond.core.utilities;

export namespace almondnamespace::utilities
{
#ifdef _WIN32
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

#define NO_RETURN_RETRY_ONCE(call)                  \
    do {                                            \
        try {                                       \
            call;                                   \
        } catch (...) {                             \
            std::cerr << "[Retry] Retrying: " #call "\n"; \
            call;                                   \
        }                                           \
    } while (0)

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
} // namespace almondnamespace::utilities
