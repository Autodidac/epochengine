module;

#include <memory>
#include <string>
#include <utility>

export module ascene;

// Engine modules
import aecs;
import almond.core.logger;
import almond.core.timing;
import amovementevent;   // MUST be a module, not a header

export namespace almondnamespace::scene
{
    using almondnamespace::timing::Timer;
    using almondnamespace::logger::Logger;
    using almondnamespace::logger::LogLevel;

    // ------------------------------------------------------------
    // SCENE
    // ------------------------------------------------------------

    export class Scene
    {
    public:
        Scene(Logger* logger,
            Timer* clock,
            LogLevel sceneLevel);

        void load();
        void unload();

        ecs::Entity createEntity();
        void destroyEntity(ecs::Entity e);

        void applyMovementEvent(const MovementEvent& ev);

        [[nodiscard]]
        std::unique_ptr<Scene> clone() const;

    private:
        void log(const std::string& msg, LogLevel lvl) const;

    private:
        ecs::registry reg{};
        bool          loaded{ false };
        Logger* logger{ nullptr };
        Timer* clock{ nullptr };
        LogLevel      sceneLogLevel{ LogLevel::INFO };
    };

    // ------------------------------------------------------------
    // IMPLEMENTATION
    // ------------------------------------------------------------

    inline Scene::Scene(Logger* L, Timer* C, LogLevel sceneLevel)
        : reg(ecs::make_registry<ecs::Position, ecs::LoggerComponent>(L, C))
        , loaded(false)
        , logger(L)
        , clock(C)
        , sceneLogLevel(sceneLevel)
    {
    }

    inline void Scene::load()
    {
        log("[Scene] Loaded", LogLevel::INFO);
        loaded = true;
    }

    inline void Scene::unload()
    {
        log("[Scene] Unloaded", LogLevel::INFO);
        loaded = false;
        reg = ecs::make_registry<ecs::Position, ecs::LoggerComponent>(nullptr, nullptr);
    }

    inline ecs::Entity Scene::createEntity()
    {
        ecs::Entity e = ecs::create_entity(reg);
        log("[Scene] Created entity " + std::to_string(e), LogLevel::INFO);
        return e;
    }

    inline void Scene::destroyEntity(ecs::Entity e)
    {
        ecs::destroy_entity(reg, e);
        log("[Scene] Destroyed entity " + std::to_string(e), LogLevel::INFO);
    }

    inline void Scene::applyMovementEvent(const MovementEvent& ev)
    {
        const ecs::Entity id = ev.getEntityId();

        if (ecs::has_component<ecs::Position>(reg, id))
        {
            auto& pos = ecs::get_component<ecs::Position>(reg, id);
            pos.x += ev.getDeltaX();
            pos.y += ev.getDeltaY();

            log(
                "[Scene] Moved entity " + std::to_string(id) +
                " by (" +
                std::to_string(ev.getDeltaX()) + ", " +
                std::to_string(ev.getDeltaY()) + ")",
                LogLevel::INFO
            );
        }
    }

    inline std::unique_ptr<Scene> Scene::clone() const
    {
        auto newScene =
            std::make_unique<Scene>(logger, clock, sceneLogLevel);

        log("[Scene] Cloned scene", LogLevel::INFO);
        // NOTE: ECS deep copy intentionally omitted
        return newScene;
    }

    inline void Scene::log(const std::string& msg, LogLevel lvl) const
    {
        if (logger && lvl >= sceneLogLevel)
            logger->log(msg, lvl);
    }

} // namespace almondnamespace::scene
