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
 // aengine.cppm (converted from legacy aengine.cpp)
 //
 // FIXES APPLIED:
 //  - No direct access to core::Context private members (ctx->hwnd).
 //    We only query windows via MultiContextManager APIs.
 //  - Removed non-constant switch case labels for ContextType::Unknown/Noop
 //    because your ContextType in your current modules is not an enum with those
 //    exact enumerators (or they’re not visible here). Default handles it.
 //
//#include "pch.h"

#include "..\include\aengine.config.hpp"
#include "..\include\aengine.hpp"
#include "aengine.backend_counts.hpp"

#if defined(_WIN32)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#endif

// -----------------------------
// Standard library imports
// -----------------------------
import <algorithm>;
import <chrono>;
import <functional>;
//import <exception>;
import <format>;
import <iostream>;
import <memory>;
import <mutex>;
import <optional>;
import <queue>;
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
//import aengine.config;

import almondshell;

import aengine.cli;
import aengine.version;
import aengine.updater;
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

import asnakelike;
import atetrislike;
import apacmanlike;
import afroggerlike;
import asokobanlike;
import amatch3like;

import aslidingpuzzlelike;
import aminesweeperlike;
import a2048like;

import asandsim;
import acellularsim;

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

namespace almondnamespace::core
{
    void RunEngine();
    void StartEngine();
    void RunEditorInterface();

    namespace engine
    {
        using PumpFunction = std::function<bool()>;

        int RunEditorInterfaceLoop(MultiContextManager& mgr, PumpFunction pump_events);
#if defined(_WIN32)
        int RunEngineMainLoopInternal(HINSTANCE hInstance, int nCmdShow);
#elif defined(__linux__)
        int RunEngineMainLoopLinux();
#endif
    }

    struct TextureUploadTask
    {
        int w{};
        int h{};
        const void* pixels{};
    };

    struct TextureUploadQueue
    {
        std::queue<TextureUploadTask> tasks;
        std::mutex mtx;

        void push(TextureUploadTask&& task)
        {
            std::lock_guard lock(mtx);
            tasks.push(std::move(task));
        }

        std::optional<TextureUploadTask> try_pop()
        {
            std::lock_guard lock(mtx);
            if (tasks.empty()) return {};
            auto task = tasks.front();
            tasks.pop();
            return task;
        }
    };

    inline std::vector<std::unique_ptr<TextureUploadQueue>> uploadQueues;

    BackendWindowCounts ResolveBackendWindowCounts()
    {
        BackendWindowCounts counts{};
        if (!cli::backend_filter)
            return counts;

        const std::string& backend = *cli::backend_filter;
        counts = BackendWindowCounts{
            .raylib = 0,
            .sdl = 0,
            .sfml = 0,
            .vulkan = 0,
            .opengl = 0,
            .software = 0
        };

        if (backend == "raylib" || backend == "raylib_nogl")
            counts.raylib = 1;
        else if (backend == "sdl")
            counts.sdl = 1;
        else if (backend == "sfml")
            counts.sfml = 1;
        else if (backend == "vulkan")
            counts.vulkan = 1;
        else if (backend == "software")
            counts.software = 1;
        else if (backend == "opengl")
            counts.opengl = 1;

        return counts;
    }

#if defined(_WIN32)
    inline void ShowConsole()
    {
#if defined(_DEBUG)
        AllocConsole();
        FILE* fp = nullptr;
        freopen_s(&fp, "CONIN$", "r", stdin);
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);
#else
        FreeConsole();
#endif
    }
#endif

    void RunEngine()
    {
#if defined(_WIN32)
        const HINSTANCE instance = GetModuleHandleW(nullptr);
        const int result = engine::RunEngineMainLoopInternal(instance, SW_SHOWNORMAL);
        if (result != 0)
            std::cerr << "[Engine] RunEngine terminated with code " << result << "\n";
#elif defined(__linux__)
        const int result = RunEngineMainLoopLinux();
        if (result != 0)
            std::cerr << "[Engine] RunEngine terminated with code " << result << "\n";
#else
        std::cerr << "[Engine] RunEngine is not implemented for this platform yet.\n";
#endif
    }

    void StartEngine()
    {
        std::cout << "AlmondShell Engine v" << almondnamespace::GetEngineVersion() << '\n';
        RunEngine();
    }

    void RunEditorInterface()
    {
#if defined(_WIN32)
        try
        {
            almondnamespace::core::MultiContextManager mgr;

            const HINSTANCE hi = GetModuleHandleW(nullptr);

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
                std::cerr << "[Editor] Failed to initialize contexts!\n";
                return;
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

            const int result = engine::RunEditorInterfaceLoop(mgr, pump);
            if (result != 0)
                std::cerr << "[Editor] RunEditorInterface terminated with code " << result << "\n";
        }
        catch (const std::exception& ex)
        {
#if defined(_DEBUG)
            almondnamespace::logger::flush_last_error_to_console();
#endif
            MessageBoxA(nullptr, ex.what(), "Error", MB_ICONERROR | MB_OK);
        }
#elif defined(__linux__)
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
                std::cerr << "[Editor] Failed to initialize contexts!\n";
                return;
            }

            input::designate_polling_thread_to_current();

            mgr.StartRenderThreads();
            mgr.ArrangeDockedWindowsGrid();

            auto pump = []() -> bool
                {
                    return almondnamespace::platform::pump_events();
                };

            const int result = engine::RunEditorInterfaceLoop(mgr, pump);
            if (result != 0)
                std::cerr << "[Editor] RunEditorInterface terminated with code " << result << "\n";
        }
        catch (const std::exception& ex)
        {
#if defined(_DEBUG)
            almondnamespace::logger::flush_last_error_to_console();
#endif
            std::cerr << "[Editor] " << ex.what() << '\n';
        }
#else
        std::cerr << "[Editor] RunEditorInterface is not implemented for this platform yet.\n";
#endif
    }
} // namespace almondnamespace::core

namespace urls
{
    const std::string github_base = "https://github.com/";
    const std::string github_raw_base = "https://raw.githubusercontent.com/";

    const std::string owner = "Autodidac/";
    const std::string repo = "Cpp_Ultimate_Project_Updater";
    const std::string branch = "main/";

    const std::string version_url = github_raw_base + owner + repo + "/" + branch + "/modules/aengine.version.ixx";
    const std::string binary_url = github_base + owner + repo + "/releases/latest/download/ConsoleApplication1.exe";
}

#if defined(_WIN32) && defined(ALMOND_USING_WINMAIN)
int WINAPI wWinMain(
    _In_     HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_     LPWSTR    lpCmdLine,
    _In_     int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

#if defined(_DEBUG)
    almondnamespace::core::ShowConsole();
#endif

    try
    {
        const int argc = __argc;
        char** argv = __argv;

        const auto cli_result = almondnamespace::core::cli::parse(argc, argv);

        const almondnamespace::updater::UpdateChannel channel{
            .version_url = urls::version_url,
            .binary_url = urls::binary_url,
        };

        if (cli_result.update_requested)
        {
            const auto update_result =
                almondnamespace::updater::run_update_command(channel, cli_result.force_update);

            if (update_result.force_required && !cli_result.force_update)
                return 2;

            return 0;
        }

        if (cli_result.editor_requested)
        {
            almondnamespace::core::RunEditorInterface();
            return 0;
        }

        return almondnamespace::core::engine::RunEngineMainLoopInternal(hInstance, SW_SHOWNORMAL);
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
#endif

int main(int argc, char** argv)
{
#if defined(_WIN32) && defined(ALMOND_USING_WINMAIN)
    return wWinMain(GetModuleHandleW(nullptr), nullptr, GetCommandLineW(), SW_SHOWNORMAL);
#else
    try
    {
        const auto cli_result = almondnamespace::core::cli::parse(argc, argv);

        const almondnamespace::updater::UpdateChannel channel{
            .version_url = urls::version_url,
            .binary_url = urls::binary_url,
        };

        if (cli_result.update_requested)
        {
            const auto update_result =
                almondnamespace::updater::run_update_command(channel, cli_result.force_update);

            if (update_result.force_required && !cli_result.force_update)
                return 2;

            return 0;
        }

        if (cli_result.editor_requested)
        {
            almondnamespace::core::RunEditorInterface();
            return 0;
        }

        almondnamespace::core::StartEngine();
        return 0;
    }
    catch (const std::exception& ex)
    {
#if defined(_DEBUG)
        almondnamespace::logger::flush_last_error_to_console();
#endif
        std::cerr << "[Fatal] " << ex.what() << '\n';
        return -1;
    }
#endif
}
