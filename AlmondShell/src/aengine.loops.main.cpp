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
 //
 // aengine.loops.main.cpp
 //

#include "..\include\aengine.config.hpp"
#include "..\include\aengine.scene_factories.hpp"
#include "aengine.backend_counts.hpp"

#if defined(_WIN32)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <include/aframework.hpp>
#endif

// -----------------------------
// Standard library imports
// -----------------------------
import <algorithm>;
import <chrono>;
import <functional>;
import <iostream>;
import <memory>;
import <mutex>;
import <optional>;
import <shared_mutex>;
import <string>;
import <string_view>;
import <thread>;
import <unordered_map>;
import <utility>;
import <vector>;

// -----------------------------
// Engine/module imports
// -----------------------------
import aengine.platform;
import almondshell;

import aengine.cli;
import aengine.input;
import aengine.engine_components;

import aengine.context.multiplexer;
import aengine.context.type;
import aengine.core.context;
import aengine.core.logger;

import aengine.gui;
import aengine.gui.menu;
import aeditor;

import ascene;

#if defined(ALMOND_USING_OPENGL)
import acontext.opengl.context;
#endif
#if defined(ALMOND_USING_SOFTWARE_RENDERER)
import acontext.softrenderer.context;
#endif
#if defined(ALMOND_USING_SDL)
import acontext.sdl.context;
#endif
#if defined(ALMOND_USING_SFML)
import acontext.sfml.context;
#endif
#if defined(ALMOND_USING_RAYLIB)
import acontext.raylib.context;
#endif

namespace input = almondnamespace::input;
namespace menu = almondnamespace::menu;
namespace gui = almondnamespace::gui;

namespace almondnamespace::core::engine
{
    using PumpFunction = std::function<bool()>;

    int RunEditorInterfaceLoop(MultiContextManager& mgr, PumpFunction pump_events);
    int RunMenuAndGamesLoop(MultiContextManager& mgr, PumpFunction pump_events);

    static int RunEngineMainLoopCommon(MultiContextManager& mgr, PumpFunction pump_events)
    {
        if (almondnamespace::core::cli::run_menu_loop)
            return RunMenuAndGamesLoop(mgr, std::move(pump_events));

        return RunEditorInterfaceLoop(mgr, std::move(pump_events));
    }

#if defined(_WIN32)
    int RunEngineMainLoopInternal(HINSTANCE hInstance, int nCmdShow)
    {
        UNREFERENCED_PARAMETER(nCmdShow);

        try
        {
            almondnamespace::core::MultiContextManager mgr;

            HINSTANCE hi = hInstance ? hInstance : GetModuleHandleW(nullptr);

            const BackendWindowCounts counts = ResolveBackendWindowCounts();
            const bool ok = mgr.Initialize(
                hi,
                /*RayLib*/   counts.raylib,
                /*SDL*/      counts.sdl,
                /*SFML*/     counts.sfml,
                /*Vulkan*/   counts.vulkan,
                /*OpenGL*/   counts.opengl,
                /*Software*/ counts.software,
                ALMOND_SINGLE_PARENT == 1
            );

            if (!ok)
            {
                return -1;
            }

            input::designate_polling_thread_to_current();

            mgr.StartRenderThreads();
            mgr.ArrangeDockedWindowsGrid();

            auto pump = []() -> bool
                {
                    MSG msg{};
                    bool keep = true;

                    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
                    {
                        if (msg.message == WM_QUIT) keep = false;
                        else
                        {
                            TranslateMessage(&msg);
                            DispatchMessageW(&msg);
                        }
                    }

                    if (!keep) return false;

                    input::poll_input();
                    return true;
                };

            return RunEngineMainLoopCommon(mgr, std::move(pump));
        }
        catch (const std::exception& ex)
        {
#if defined(_DEBUG)
            almondnamespace::logger::flush_last_error_to_console();
#endif
            MessageBoxA(nullptr, ex.what(), "Error", MB_ICONERROR | MB_OK);
            return -1;
        }
    }
#elif defined(__linux__)
    int RunEngineMainLoopLinux()
    {
        try
        {
            almondnamespace::core::MultiContextManager mgr;

            const BackendWindowCounts counts = ResolveBackendWindowCounts();
            const bool ok = mgr.Initialize(
                nullptr,
                /*RayLib*/   counts.raylib,
                /*SDL*/      counts.sdl,
                /*SFML*/     counts.sfml,
                /*Vulkan*/   counts.vulkan,
                /*OpenGL*/   counts.opengl,
                /*Software*/ counts.software,
                ALMOND_SINGLE_PARENT == 1
            );

            if (!ok)
            {
                std::cerr << "[Engine] Failed to initialize contexts!\n";
                return -1;
            }

            input::designate_polling_thread_to_current();

            mgr.StartRenderThreads();
            mgr.ArrangeDockedWindowsGrid();

            auto pump = []() -> bool
                {
                    return almondnamespace::platform::pump_events();
                };

            return RunEngineMainLoopCommon(mgr, std::move(pump));
        }
        catch (const std::exception& ex)
        {
#if defined(_DEBUG)
            almondnamespace::logger::flush_last_error_to_console();
#endif
            std::cerr << "[Engine] " << ex.what() << '\n';
            return -1;
        }
    }
#endif
} // namespace almondnamespace::core::engine
