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
 // aengine.loops.editor.cpp
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
} // namespace almondnamespace::core::engine
