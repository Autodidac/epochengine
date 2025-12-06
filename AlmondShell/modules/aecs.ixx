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
export module aecs;

import std;
import <format>;
import <string>;
import <string_view>;
import <typeinfo>;
import <utility>;

export import :components;
export import :storage;

import almond.core.time;
import almond.core.logger;

export namespace almondnamespace::ecs
{
    template<typename... Cs>
    struct reg_ex
    {
        ComponentStorage storage;
        Entity nextID{ 1 };
        almondnamespace::Logger* log{ nullptr };
        almondnamespace::timing::Timer* clk{ nullptr };
    };

    template<typename... Cs>
    [[nodiscard]] inline reg_ex<Cs...> make_registry(
        almondnamespace::Logger* L = nullptr,
        almondnamespace::timing::Timer* C = nullptr)
    {
        return { {}, 1, L, C };
    }

    namespace _detail
    {
        inline void notify(almondnamespace::Logger* log,
            almondnamespace::timing::Timer* clk,
            Entity e,
            std::string_view action,
            std::string_view comp = "")
        {
            if (!log || !clk) return;

            auto ts = timing::getCurrentTimeString();
            log->log(std::format("[ECS] {}{} entity={} at {}",
                action,
                comp.empty() ? "" : std::format(":{}", comp),
                e, ts));

            events::push_event(events::Event{
                events::EventType::Custom,
                { {"ecs_action", std::string(action)},
                  {"entity",     std::to_string(e)},
                  {"component",  std::string(comp)},
                  {"timing",       ts} },
                0.f, 0.f });
        }
    }

    template<typename... Cs>
    inline Entity create_entity(reg_ex<Cs...>& R)
    {
        Entity e = R.nextID++;
        _detail::notify(R.log, R.clk, e, "createEntity");
        return e;
    }

    template<typename... Cs>
    inline void destroy_entity(reg_ex<Cs...>& R, Entity e)
    {
        (remove_component<Cs>(R.storage, e), ...);
        _detail::notify(R.log, R.clk, e, "destroyEntity");
    }

    template<typename C, typename... Cs>
    inline void add_component(reg_ex<Cs...>& R, Entity e, C c)
    {
        ::almondnamespace::ecs::add_component<C>(R.storage, e, std::move(c));
        _detail::notify(R.log, R.clk, e, "addComponent", typeid(C).name());
    }

    template<typename C, typename... Cs>
    inline void remove_component(reg_ex<Cs...>& R, Entity e)
    {
        ::almondnamespace::ecs::remove_component<C>(R.storage, e);
        _detail::notify(R.log, R.clk, e, "removeComponent", typeid(C).name());
    }

    template<typename C, typename... Cs>
    [[nodiscard]] inline bool has_component(const reg_ex<Cs...>& R, Entity e)
    {
        return ::almondnamespace::ecs::has_component<C>(R.storage, e);
    }

    template<typename C, typename... Cs>
    [[nodiscard]] inline C& get_component(reg_ex<Cs...>& R, Entity e)
    {
        return ::almondnamespace::ecs::get_component<C>(R.storage, e);
    }

    template<typename... Vs, typename... Cs, typename Fn>
    inline void view(reg_ex<Cs...>& R, Fn&& fn)
    {
        for (auto& [ent, compMap] : R.storage)
        {
            (void)compMap;
            if ((has_component<Vs>(R, ent) && ...))
            {
                fn(ent, get_component<Vs>(R, ent)...);
            }
        }
    }
}
