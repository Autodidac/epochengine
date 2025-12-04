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

import <cassert>;
import <string_view>;
import <typeinfo>;
import <utility>;

import "aentitycomponentmanager.hpp";
#include "aentity.hpp";

import almond.core.time;

namespace almondnamespace
{
    class Logger;
    namespace time { class Timer; }
}

export namespace almondnamespace::ecs
{
    using Entity = EntityID;

    template<typename... Cs>
    struct reg_ex
    {
        ComponentStorage storage;
        EntityID nextID{ 1 };
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
        void notify(almondnamespace::Logger* log,
            almondnamespace::timing::Timer* clk,
            Entity e,
            std::string_view action,
            std::string_view comp = "");
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
