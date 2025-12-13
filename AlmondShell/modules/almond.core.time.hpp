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

#include <chrono>
#include <format>
#include <string>
#include <string_view>
#include <unordered_map>

#ifndef ALMOND_TIME_EXPORT
#define ALMOND_TIME_EXPORT
#endif

namespace almondnamespace::timing
{
    ALMOND_TIME_EXPORT using Clock = std::chrono::steady_clock;

    struct ALMOND_TIME_EXPORT Timer
    {
        Clock::time_point baseStart{ Clock::now() };
        double accumulatedTime = 0.0;
        double timeScale = 1.0;
        bool paused = false;
        double manualElapsed = 0.0;
    };

    ALMOND_TIME_EXPORT inline Timer createTimer(double scale = 1.0)
    {
        Timer t;
        t.timeScale = scale;
        return t;
    }

    ALMOND_TIME_EXPORT inline double elapsed(const Timer& t)
    {
        if (t.paused) return t.accumulatedTime;
        auto now = Clock::now();
        auto realElapsed = std::chrono::duration<double>(now - t.baseStart).count();
        return t.accumulatedTime + (realElapsed * t.timeScale) + t.manualElapsed;
    }

    ALMOND_TIME_EXPORT inline double realElapsed(const Timer& t)
    {
        if (t.paused) return t.accumulatedTime / t.timeScale;
        return std::chrono::duration<double>(Clock::now() - t.baseStart).count();
    }

    ALMOND_TIME_EXPORT inline void pause(Timer& t)
    {
        if (!t.paused) {
            t.accumulatedTime = elapsed(t);
            t.paused = true;
        }
    }

    ALMOND_TIME_EXPORT inline void resume(Timer& t)
    {
        if (t.paused) {
            t.baseStart = Clock::now();
            t.paused = false;
        }
    }

    ALMOND_TIME_EXPORT inline void reset(Timer& t)
    {
        t.baseStart = Clock::now();
        t.accumulatedTime = 0.0;
        t.manualElapsed = 0.0;
        t.paused = false;
    }

    ALMOND_TIME_EXPORT inline void advance(Timer& t, double dt)
    {
        t.manualElapsed += dt * t.timeScale;
    }

    ALMOND_TIME_EXPORT inline void setScale(Timer& t, double newScale)
    {
        auto unscaled = elapsed(t);
        t.timeScale = newScale;
        t.baseStart = Clock::now();
        t.accumulatedTime = unscaled;
        t.manualElapsed = 0.0;
    }

    struct ALMOND_TIME_EXPORT ScopedTimers
    {
        std::unordered_map<std::string, Timer> timers;
    };

    ALMOND_TIME_EXPORT inline std::unordered_map<std::string, ScopedTimers>& timerRegistry()
    {
        static std::unordered_map<std::string, ScopedTimers> registry;
        return registry;
    }

    ALMOND_TIME_EXPORT inline Timer& createNamedTimer(std::string_view group, std::string_view name, double scale = 1.0)
    {
        return timerRegistry()[std::string(group)].timers[std::string(name)] = createTimer(scale);
    }

    ALMOND_TIME_EXPORT inline Timer* getTimer(std::string_view group, std::string_view name)
    {
        auto it = timerRegistry().find(std::string(group));
        if (it != timerRegistry().end()) {
            auto jt = it->second.timers.find(std::string(name));
            if (jt != it->second.timers.end()) return &jt->second;
        }
        return nullptr;
    }

    ALMOND_TIME_EXPORT inline void removeTimer(std::string_view group, std::string_view name)
    {
        auto it = timerRegistry().find(std::string(group));
        if (it != timerRegistry().end()) {
            it->second.timers.erase(std::string(name));
            if (it->second.timers.empty())
                timerRegistry().erase(it);
        }
    }

    ALMOND_TIME_EXPORT inline std::string getCurrentTimeString()
    {
        const auto now = std::chrono::system_clock::now();
        const auto local = std::chrono::zoned_time{ std::chrono::current_zone(), now };
        return std::format("{:%Y-%m-%d %H:%M:%S}", local);
    }
} // namespace almondnamespace::timing

#undef ALMOND_TIME_EXPORT
