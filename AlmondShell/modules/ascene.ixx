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

export module ascene;

import <memory>;
import <string>;

import aecs;
import almond.core.logger;
import almond.core.time;
import aengine;
import "amovementevent.hpp";


namespace almondnamespace::scene
{
    template <typename... Components>
    struct SceneComponentList
    {
        using Registry = almondnamespace::ecs::reg_ex<Components...>;

        static Registry make_registry(almondnamespace::Logger* logger,
            almondnamespace::timing::Timer* clock)
        {
            return almondnamespace::ecs::make_registry<Components...>(logger, clock);
        }
    };

    // Override this alias to change the default component set carried by Scene registries.
    using DefaultSceneComponents = SceneComponentList<ecs::Position>;

    class Scene
    {
    public:
        using Components = DefaultSceneComponents;
        using Registry = typename Components::Registry;

        Scene(Logger* L = nullptr,
            timing::Timer* C = nullptr,
            LogLevel sceneLevel = LogLevel::INFO);

        Scene(const Scene&) = delete;
        Scene& operator=(const Scene&) = delete;
        Scene(Scene&&) noexcept = default;
        Scene& operator=(Scene&&) noexcept = default;

        virtual ~Scene() = default;

        // Lifecycle
        virtual void load();
        virtual void unload();

        // Per-frame hook (override in derived game scenes)
        virtual bool frame(std::shared_ptr<almondnamespace::core::Context>,
            almondnamespace::core::WindowData*)
        {
            return true; // default: no-op
        }

        // Entity management
        ecs::Entity createEntity();
        void destroyEntity(ecs::Entity e);

        // Apply external event
        void applyMovementEvent(const almondnamespace::MovementEvent& ev);

        // Clone
        virtual std::unique_ptr<Scene> clone() const;

        bool isLoaded() const { return loaded; }

        Registry& registry() { return reg; }
        const Registry& registry() const { return reg; }

        void setLogLevel(LogLevel lvl) { sceneLogLevel = lvl; }
        LogLevel getLogLevel() const { return sceneLogLevel; }

    protected:
        void log(const std::string& msg, LogLevel lvl) const;

    private:
        Registry reg;
        bool loaded = false;
        Logger* logger = nullptr;       // optional shared logger
        timing::Timer* clock = nullptr;   // optional time reference
        LogLevel sceneLogLevel{ LogLevel::INFO };         // per-scene verbosity threshold
    };

} // namespace almondnamespace::scene
