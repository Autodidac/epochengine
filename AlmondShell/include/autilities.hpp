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
#pragma once

#include "aplatform.hpp"

#include <iostream>

#if defined(__cpp_modules) && __cpp_modules >= 201907L && !defined(ALMOND_FORCE_LEGACY_HEADERS)
import almond.core.utilities;
#else
#include <type_traits>
#include <utility>

namespace almondnamespace::utilities
{
#ifdef _WIN32 // Windows-specific

#include <consoleapi3.h>
#include "aframework.hpp"

    [[nodiscard]] inline constexpr bool isConsoleApplication() noexcept
    {
        return GetConsoleWindow() != nullptr;
    }

#else // Unix-like

#include <unistd.h>
#include <stdio.h>

    [[nodiscard]] inline constexpr bool isConsoleApplication() noexcept
    {
        return isatty(fileno(stdout)) != 0;
    }

#endif

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
#endif

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
