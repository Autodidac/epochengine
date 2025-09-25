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

#include "aplatform.hpp"      // must always come first
#include "acontextstate.hpp"  // ContextState definition

#include <memory>
#include <stdexcept>
#include <utility>

namespace almondnamespace::state
{
    struct ContextState; // Forward declaration
}

namespace almondnamespace
{
    class ContextManager
    {
    public:
        static void setContext(std::shared_ptr<state::ContextState> ctx)
        {
            instance() = std::move(ctx);
        }

        static state::ContextState& getContext()
        {
            auto& inst = instance();
            if (!inst)
                throw std::runtime_error("No active ContextState set");
            return *inst;
        }

        static void resetContext()
        {
            instance().reset();
        }

        static bool hasContext()
        {
            return static_cast<bool>(instance());
        }

        static std::shared_ptr<state::ContextState> tryGetContext()
        {
            return instance();
        }

        class ContextScope
        {
        public:
            explicit ContextScope(std::shared_ptr<state::ContextState> ctx)
                : previous(ContextManager::replaceContext(std::move(ctx)))
            {
            }

            ContextScope(const ContextScope&) = delete;
            ContextScope& operator=(const ContextScope&) = delete;

            ContextScope(ContextScope&&) = delete;
            ContextScope& operator=(ContextScope&&) = delete;

            ~ContextScope()
            {
                ContextManager::replaceContext(std::move(previous));
            }

        private:
            std::shared_ptr<state::ContextState> previous;
        };

    private:
        static std::shared_ptr<state::ContextState>& instance()
        {
            static thread_local std::shared_ptr<state::ContextState> s_instance;
            return s_instance;
        }

        static std::shared_ptr<state::ContextState> replaceContext(std::shared_ptr<state::ContextState> ctx)
        {
            auto& inst = instance();
            std::swap(inst, ctx);
            return ctx;
        }
    };
}
