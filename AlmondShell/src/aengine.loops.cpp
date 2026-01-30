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
 // aengine.loops.cpp
 //

#include "..\include\aengine.config.hpp"
#include "..\include\aengine.scene_factories.hpp"
#include "..\include\aengine.backend_counts.hpp"

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
//import almondshell;

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

    int RunEditorInterfaceLoop(MultiContextManager& mgr, PumpFunction pump_events)
    {
        enum class EditorSceneState
        {
            Editor,
            Game,
            Exit
        };

        EditorSceneState state = EditorSceneState::Editor;
        std::unique_ptr<almondnamespace::scene::Scene> active_scene{};

        using MenuOverlay = almondnamespace::menu::MenuOverlay;
        using EditorCommandOverlay = almondnamespace::menu::EditorCommandOverlay;
        MenuOverlay games_menu{};
        EditorCommandOverlay editor_menu{};

        games_menu.set_max_columns(almondnamespace::core::cli::menu_columns);
        editor_menu.initialize();

        auto collect_backend_contexts = []()
            {
                using ContextGroup = std::pair<
                    almondnamespace::core::ContextType,
                    std::vector<std::shared_ptr<almondnamespace::core::Context>>
                >;

                std::vector<ContextGroup> snapshot;

                {
                    std::shared_lock lock(almondnamespace::core::g_backendsMutex);
                    snapshot.reserve(almondnamespace::core::g_backends.size());

                    for (auto& [type, state] : almondnamespace::core::g_backends)
                    {
                        std::vector<std::shared_ptr<almondnamespace::core::Context>> contexts;
                        contexts.reserve(1 + state.duplicates.size());

                        if (state.master) contexts.push_back(state.master);
                        for (auto& dup : state.duplicates) contexts.push_back(dup);

                        snapshot.emplace_back(type, std::move(contexts));
                    }
                }

                return snapshot;
            };

        auto init_menus = [&]()
            {
                auto snapshot = collect_backend_contexts();
                for (auto& [_, contexts] : snapshot)
                    for (auto& ctx : contexts)
                    {
                        if (ctx)
                        {
                            games_menu.initialize_game_choices();
                            games_menu.recompute_layout(ctx, ctx->get_width_safe(), ctx->get_height_safe());
                        }
                    }
            };

        init_menus();

        std::unordered_map<Context*, std::chrono::steady_clock::time_point> last_frame_times;
        bool running = true;
        bool show_games_popup = false;
        PumpFunction pump = std::move(pump_events);

        while (running)
        {
            if (!pump())
            {
                running = false;
                break;
            }

            mgr.CleanupFinishedWindows();

            auto snapshot = collect_backend_contexts();
#if !defined(ALMOND_SINGLE_PARENT)
            bool any_context_alive = false;
#endif
            for (auto& [type, contexts] : snapshot)
            {
                auto update_on_ctx = [&](std::shared_ptr<Context> ctx) -> bool
                    {
                        if (!ctx) return true;

                        auto* win = mgr.findWindowByContext(ctx);
                        if (!win) return true;

                        bool ctx_running = win->running;

                        const auto now = std::chrono::steady_clock::now();
                        const auto raw = ctx.get();

                        float dt = 0.0f;
                        auto [it, inserted] = last_frame_times.emplace(raw, now);
                        if (!inserted)
                        {
                            dt = std::chrono::duration<float>(now - it->second).count();
                            it->second = now;
                        }

                        auto begin_scene = [&](auto make_scene, const char* label)
                            {
                                auto clear_commands = [](const std::shared_ptr<Context>& c)
                                    {
                                        if (c && c->windowData) c->windowData->commandQueue.clear();
                                    };

                                auto snap2 = collect_backend_contexts();
                                for (auto& [__, group] : snap2)
                                    for (auto& c : group)
                                        clear_commands(c);

                                games_menu.cleanup();

                                if (active_scene)
                                    active_scene->unload();

                                active_scene = make_scene();
                                active_scene->load();
                                // state = EditorSceneState::Game;
                                state = EditorSceneState::Editor;
                                show_games_popup = false;
                                std::cout << "[Editor] Launching " << label << " scene.\n";
                            };

                        if (state == EditorSceneState::Editor)
                        {
                            int mx = 0, my = 0;
                            ctx->get_mouse_position_safe(mx, my);

                            const gui::Vec2 mouse_pos{
                                static_cast<float>(mx),
                                static_cast<float>(my)
                            };

                            const bool mouse_left_down =
                                almondnamespace::input::mouseDown.test(almondnamespace::input::MouseButton::MouseLeft);

                            const bool up_pressed =
                                almondnamespace::input::keyPressed.test(almondnamespace::input::Key::Up);
                            const bool down_pressed =
                                almondnamespace::input::keyPressed.test(almondnamespace::input::Key::Down);
                            const bool left_pressed =
                                almondnamespace::input::keyPressed.test(almondnamespace::input::Key::Left);
                            const bool right_pressed =
                                almondnamespace::input::keyPressed.test(almondnamespace::input::Key::Right);
                            const bool enter_pressed =
                                almondnamespace::input::keyPressed.test(almondnamespace::input::Key::Enter);

                            ctx->clear_safe();
                            gui::begin_frame(ctx, dt, mouse_pos, mouse_left_down);
                            gui::WidgetBounds editor_bounds{};
                            const bool editor_clicked = almondnamespace::editor_run(ctx, &editor_bounds);
                            if (editor_clicked)
                                show_games_popup = !show_games_popup;

                            const bool draw_editor_overlay = !show_games_popup;
                            const bool menu_has_focus = draw_editor_overlay;
                            std::optional<almondnamespace::menu::EditorCommandChoice> command_choice{};
                            if (draw_editor_overlay)
                            {
                                command_choice = editor_menu.update_and_draw(
                                    ctx,
                                    win,
                                    dt,
                                    menu_has_focus ? up_pressed : false,
                                    menu_has_focus ? down_pressed : false,
                                    menu_has_focus ? enter_pressed : false,
                                    menu_has_focus,
                                    editor_bounds);
                            }

                            if (command_choice)
                            {
                                using almondnamespace::menu::EditorCommandChoice;

                                switch (*command_choice)
                                {
                                case EditorCommandChoice::OpenProject:
                                    std::cout << "[Editor] Open Project selected.\n";
                                    break;
                                case EditorCommandChoice::Settings:
                                    std::cout << "[Editor] Settings selected.\n";
                                    break;
                                case EditorCommandChoice::RunGame:
                                    show_games_popup = !show_games_popup;
                                    break;
                                case EditorCommandChoice::Exit:
                                    state = EditorSceneState::Exit;
                                    running = false;
                                    break;
                                }
                            }

                            if (show_games_popup)
                            {
                                const float popup_width = (std::max)(600.0f, ctx->get_width_safe() * 0.7f);
                                const float popup_height = (std::max)(360.0f, ctx->get_height_safe() * 0.6f);

                                const gui::Vec2 popup_size{ popup_width, popup_height };
                                const gui::Vec2 popup_pos{
                                    (ctx->get_width_safe() - popup_size.x) * 0.5f,
                                    (ctx->get_height_safe() - popup_size.y) * 0.5f
                                };

                                auto game_choice = games_menu.update_and_draw_in_window(
                                    ctx,
                                    win,
                                    dt,
                                    up_pressed,
                                    down_pressed,
                                    left_pressed,
                                    right_pressed,
                                    enter_pressed,
                                    "Games",
                                    popup_pos,
                                    popup_size,
                                    true);

                                if (game_choice)
                                {
                                    using almondnamespace::menu::Choice;

                                    if (*game_choice == Choice::Snake)
                                        begin_scene(CreateSnakeScene, "Snake");
                                    else if (*game_choice == Choice::Tetris)
                                        begin_scene(CreateTetrisScene, "Tetris");
                                    else if (*game_choice == Choice::Frogger)
                                        begin_scene(CreateFroggerScene, "Frogger");
                                    else if (*game_choice == Choice::Pacman)
                                        begin_scene(CreatePacmanScene, "Pacman");
                                    else if (*game_choice == Choice::Sokoban)
                                        begin_scene(CreateSokobanScene, "Sokoban");
                                    else if (*game_choice == Choice::Bejeweled)
                                        begin_scene(CreateMatch3Scene, "Match-3");
                                    else if (*game_choice == Choice::Puzzle)
                                        begin_scene(CreateSlidingPuzzleScene, "Sliding Puzzle");
                                    else if (*game_choice == Choice::Minesweep)
                                        begin_scene(CreateMinesweeperScene, "Minesweeper");
                                    else if (*game_choice == Choice::Fourty)
                                        begin_scene(Create2048Scene, "2048");
                                    else if (*game_choice == Choice::Sandsim)
                                        begin_scene(CreateSandSimScene, "Sand Sim");
                                    else if (*game_choice == Choice::Cellular)
                                        begin_scene(CreateCellularSimScene, "Cellular");
                                }
                            }

                            gui::end_frame();
                            ctx->present_safe();
                        }
                        else if (state == EditorSceneState::Game)
                        {
                            if (active_scene)
                            {
                                ctx_running = active_scene->frame(ctx, win);
                                if (!ctx_running)
                                {
                                    active_scene->unload();
                                    active_scene.reset();
                                    state = EditorSceneState::Editor;
                                    games_menu.cleanup();
                                    games_menu.initialize_game_choices();
                                    editor_menu.reset_selection();
                                }
                            }
                            else
                            {
                                state = EditorSceneState::Editor;
                            }
                        }
                        else if (state == EditorSceneState::Exit)
                        {
                            running = false;
                        }

                        if (!ctx_running)
                            last_frame_times.erase(raw);

                        return ctx_running;
                    };

#if defined(ALMOND_SINGLE_PARENT)
                if (!contexts.empty())
                {
                    auto master = contexts.front();
                    if (master && !update_on_ctx(master)) { running = false; break; }

                    for (std::size_t i = 1; i < contexts.size(); ++i)
                        if (!update_on_ctx(contexts[i])) running = false;
                }
#else
                bool any_alive = false;
                for (std::size_t i = 0; i < contexts.size(); ++i)
                {
                    auto& ctx = contexts[i];
                    if (!ctx) continue;

                    const bool alive = update_on_ctx(ctx);
                    if (alive) any_alive = true;
                }
                if (any_alive) any_context_alive = true;
#endif
                if (!running) break;
            }
#if !defined(ALMOND_SINGLE_PARENT)
            if (!any_context_alive) running = false;
#endif

            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }

        if (active_scene)
        {
            active_scene->unload();
            active_scene.reset();
        }

        games_menu.cleanup();

        auto snapshot2 = collect_backend_contexts();
        for (auto& [type, contexts] : snapshot2)
        {
            auto cleanup_backend = [&](std::shared_ptr<almondnamespace::core::Context> ctx)
                {
                    if (!ctx) return;

                    switch (type)
                    {
#if defined(ALMOND_USING_OPENGL)
                    case almondnamespace::core::ContextType::OpenGL:
                        almondnamespace::openglcontext::opengl_cleanup(ctx);
                        break;
#endif
#if defined(ALMOND_USING_SOFTWARE_RENDERER)
                    case almondnamespace::core::ContextType::Software:
                        // almondnamespace::anativecontext::softrenderer_cleanup(ctx);
                        break;
#endif
#if defined(ALMOND_USING_SDL)
                    case almondnamespace::core::ContextType::SDL:
                        //  almondnamespace::sdlcontext::sdl_cleanup(ctx);
                        break;
#endif
#if defined(ALMOND_USING_SFML)
                    case almondnamespace::core::ContextType::SFML:
                        almondnamespace::sfmlcontext::sfml_cleanup(ctx);
                        break;
#endif
#if defined(ALMOND_USING_RAYLIB)
                    case almondnamespace::core::ContextType::RayLib:
                        almondnamespace::raylibcontext::raylib_cleanup(ctx);
                        break;
#endif
                    case almondnamespace::core::ContextType::Noop:
                        break;
                    default:
                        break;
                    }
                };

            for (auto& ctx : contexts) cleanup_backend(ctx);
        }

        mgr.StopAll();

        return 0;
    }

    int RunMenuAndGamesLoop(MultiContextManager& mgr, PumpFunction pump_events)
    {
        enum class SceneID
        {
            Menu,
            Snake,
            Tetris,
            Pacman,
            Frogger,
            Sokoban,
            Match3,
            Sliding,
            Minesweeper,
            Game2048,
            Sandsim,
            Cellular,
            Exit
        };

        SceneID scene_id = SceneID::Menu;
        std::unique_ptr<almondnamespace::scene::Scene> active_scene{};

        using MenuOverlay = almondnamespace::menu::MenuOverlay;
        MenuOverlay menu{};
        menu.set_max_columns(almondnamespace::core::cli::menu_columns);

        auto collect_backend_contexts = []()
            {
                using ContextGroup = std::pair<
                    almondnamespace::core::ContextType,
                    std::vector<std::shared_ptr<almondnamespace::core::Context>>
                >;

                std::vector<ContextGroup> snapshot;

                {
                    std::shared_lock lock(almondnamespace::core::g_backendsMutex);
                    snapshot.reserve(almondnamespace::core::g_backends.size());

                    for (auto& [type, state] : almondnamespace::core::g_backends)
                    {
                        std::vector<std::shared_ptr<almondnamespace::core::Context>> contexts;
                        contexts.reserve(1 + state.duplicates.size());

                        if (state.master) contexts.push_back(state.master);
                        for (auto& dup : state.duplicates) contexts.push_back(dup);

                        snapshot.emplace_back(type, std::move(contexts));
                    }
                }

                return snapshot;
            };

        auto init_menu = [&]()
            {
                auto snapshot = collect_backend_contexts();
                for (auto& [_, contexts] : snapshot)
                    for (auto& ctx : contexts)
                        if (ctx) menu.initialize(ctx);
            };

        init_menu();

        std::unordered_map<Context*, std::chrono::steady_clock::time_point> last_frame_times;
        bool running = true;
        PumpFunction pump = std::move(pump_events);

        while (running)
        {
            if (!pump())
            {
                running = false;
                break;
            }

            mgr.CleanupFinishedWindows();

            auto snapshot = collect_backend_contexts();
#if !defined(ALMOND_SINGLE_PARENT)
            bool any_context_alive = false;
#endif
            for (auto& [type, contexts] : snapshot)
            {
                auto update_on_ctx = [&](std::shared_ptr<Context> ctx) -> bool
                    {
                        if (!ctx) return true;

                        // FIX: never touch ctx->hwnd (private). Manager can resolve by context.
                        auto* win = mgr.findWindowByContext(ctx);
                        if (!win) return true;

                        bool ctx_running = win->running;

                        const auto now = std::chrono::steady_clock::now();
                        const auto raw = ctx.get();

                        float dt = 0.0f;
                        auto [it, inserted] = last_frame_times.emplace(raw, now);
                        if (!inserted)
                        {
                            dt = std::chrono::duration<float>(now - it->second).count();
                            it->second = now;
                        }

                        auto begin_scene = [&](auto make_scene, SceneID id)
                            {
                                auto clear_commands = [](const std::shared_ptr<Context>& c)
                                    {
                                        // If your WindowData/queue moved behind accessors, change this here.
                                        if (c && c->windowData) c->windowData->commandQueue.clear();
                                    };

                                auto snap2 = collect_backend_contexts();
                                for (auto& [__, group] : snap2)
                                    for (auto& c : group)
                                        clear_commands(c);

                                menu.cleanup();

                                if (active_scene)
                                    active_scene->unload();

                                active_scene = make_scene();
                                active_scene->load();
                                scene_id = id;
                            };

                        switch (scene_id)
                        {
                        case SceneID::Menu:
                        {
                            int mx = 0, my = 0;
                            ctx->get_mouse_position_safe(mx, my);

                            const gui::Vec2 mouse_pos{
                                static_cast<float>(mx),
                                static_cast<float>(my)
                            };

                            const bool mouse_left_down =
                                almondnamespace::input::mouseDown.test(almondnamespace::input::MouseButton::MouseLeft);

                            const bool up_pressed =
                                almondnamespace::input::keyPressed.test(almondnamespace::input::Key::Up);
                            const bool down_pressed =
                                almondnamespace::input::keyPressed.test(almondnamespace::input::Key::Down);
                            const bool left_pressed =
                                almondnamespace::input::keyPressed.test(almondnamespace::input::Key::Left);
                            const bool right_pressed =
                                almondnamespace::input::keyPressed.test(almondnamespace::input::Key::Right);
                            const bool enter_pressed =
                                almondnamespace::input::keyPressed.test(almondnamespace::input::Key::Enter);

                            ctx->clear_safe();
                            gui::begin_frame(ctx, dt, mouse_pos, mouse_left_down);
                            auto choice = menu.update_and_draw(ctx, win, dt, up_pressed, down_pressed, left_pressed, right_pressed, enter_pressed);
                            gui::end_frame();
                            ctx->present_safe();

                            if (choice)
                            {
                                using almondnamespace::menu::Choice;

                                if (*choice == Choice::Snake)
                                    begin_scene(CreateSnakeScene, SceneID::Snake);
                                else if (*choice == Choice::Tetris)
                                    begin_scene(CreateTetrisScene, SceneID::Tetris);
                                else if (*choice == Choice::Frogger)
                                    begin_scene(CreateFroggerScene, SceneID::Frogger);
                                else if (*choice == Choice::Pacman)
                                    begin_scene(CreatePacmanScene, SceneID::Pacman);
                                else if (*choice == Choice::Sokoban)
                                    begin_scene(CreateSokobanScene, SceneID::Sokoban);
                                else if (*choice == Choice::Bejeweled)
                                    begin_scene(CreateMatch3Scene, SceneID::Match3);
                                else if (*choice == Choice::Puzzle)
                                    begin_scene(CreateSlidingPuzzleScene, SceneID::Sliding);
                                else if (*choice == Choice::Minesweep)
                                    begin_scene(CreateMinesweeperScene, SceneID::Minesweeper);
                                else if (*choice == Choice::Fourty)
                                    begin_scene(Create2048Scene, SceneID::Game2048);
                                else if (*choice == Choice::Sandsim)
                                    begin_scene(CreateSandSimScene, SceneID::Sandsim);
                                else if (*choice == Choice::Cellular)
                                    begin_scene(CreateCellularSimScene, SceneID::Cellular);
                                else if (*choice == Choice::Settings)
                                    std::cout << "[Menu] Settings selected.\n";
                                else if (*choice == Choice::Exit)
                                {
                                    scene_id = SceneID::Exit;
                                    running = false;
                                }
                            }
                            break;
                        }

                        // cascading case to reset to menu after game exit
                        case SceneID::Snake:
                        case SceneID::Tetris:
                        case SceneID::Pacman:
                        case SceneID::Frogger:
                        case SceneID::Sokoban:
                        case SceneID::Match3:
                        case SceneID::Sliding:
                        case SceneID::Minesweeper:
                        case SceneID::Game2048:
                        case SceneID::Sandsim:
                        case SceneID::Cellular:
                        {
                            if (active_scene)
                            {
                                ctx_running = active_scene->frame(ctx, win);
                                if (!ctx_running)
                                {
                                    active_scene->unload();
                                    active_scene.reset();
                                    scene_id = SceneID::Menu;
                                    init_menu();
                                }
                            }
                            break;
                        }

                        case SceneID::Exit:
                            running = false;
                            break;
                        }

                        if (!ctx_running)
                            last_frame_times.erase(raw);

                        return ctx_running;
                    };

#if defined(ALMOND_SINGLE_PARENT)
                if (!contexts.empty())
                {
                    auto master = contexts.front();
                    if (master && !update_on_ctx(master)) { running = false; break; }

                    for (std::size_t i = 1; i < contexts.size(); ++i)
                        if (!update_on_ctx(contexts[i])) running = false;
                }
#else
                bool any_alive = false;
                for (std::size_t i = 0; i < contexts.size(); ++i)
                {
                    auto& ctx = contexts[i];
                    if (!ctx) continue;

                    const bool alive = update_on_ctx(ctx);
                    if (alive) any_alive = true;
                }
                if (any_alive) any_context_alive = true;
#endif
                if (!running) break;
            }
#if !defined(ALMOND_SINGLE_PARENT)
            if (!any_context_alive) running = false;
#endif

            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }

        if (active_scene)
        {
            active_scene->unload();
            active_scene.reset();
        }

        menu.cleanup();

        // Backend cleanup
        auto snapshot2 = collect_backend_contexts();
        for (auto& [type, contexts] : snapshot2)
        {
            auto cleanup_backend = [&](std::shared_ptr<almondnamespace::core::Context> ctx)
                {
                    if (!ctx) return;

                    switch (type)
                    {
#if defined(ALMOND_USING_OPENGL)
                    case almondnamespace::core::ContextType::OpenGL:
                        almondnamespace::openglcontext::opengl_cleanup(ctx);
                        break;
#endif
#if defined(ALMOND_USING_SOFTWARE_RENDERER)
                    case almondnamespace::core::ContextType::Software:
                        // almondnamespace::anativecontext::softrenderer_cleanup(ctx);
                        break;
#endif
#if defined(ALMOND_USING_SDL)
                    case almondnamespace::core::ContextType::SDL:
                        //  almondnamespace::sdlcontext::sdl_cleanup(ctx);
                        break;
#endif
#if defined(ALMOND_USING_SFML)
                    case almondnamespace::core::ContextType::SFML:
                        almondnamespace::sfmlcontext::sfml_cleanup(ctx);
                        break;
#endif
#if defined(ALMOND_USING_RAYLIB)
                    case almondnamespace::core::ContextType::RayLib:
                        almondnamespace::raylibcontext::raylib_cleanup(ctx);
                        break;
#endif


                    case almondnamespace::core::ContextType::Noop:
                        break;
                    default:
                        break;
                    }
                };

            for (auto& ctx : contexts) cleanup_backend(ctx);
        }

        mgr.StopAll();

        return 0;
    }

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
                //MessageBoxW(nullptr, L"Failed to initialize contexts!", L"Error", MB_ICONERROR | MB_OK);
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
