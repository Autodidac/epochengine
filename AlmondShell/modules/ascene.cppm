module;

import <string>;

import ascene;
import aecs;
import almond.core.logger;
import almond.core.time;
import aengine:platform;
import "aentitycomponents.hpp";
import "amovementevent.hpp";

namespace almondnamespace::scene
{
    Scene::Scene(Logger* L, time::Timer* C, LogLevel sceneLevel)
        : reg(Components::make_registry(L, C)),
        logger(L),
        clock(C),
        sceneLogLevel(sceneLevel)
    {
    }

    void Scene::load()
    {
        log("[Scene] Loaded", LogLevel::INFO);
        loaded = true;
    }

    void Scene::unload()
    {
        log("[Scene] Unloaded", LogLevel::INFO);
        loaded = false;
        reg = Components::make_registry(nullptr, nullptr);
    }

    ecs::Entity Scene::createEntity()
    {
        ecs::Entity e = ecs::create_entity(reg);
        log("[Scene] Created entity " + std::to_string(e), LogLevel::INFO);
        return e;
    }

    void Scene::destroyEntity(ecs::Entity e)
    {
        ecs::destroy_entity(reg, e);
        log("[Scene] Destroyed entity " + std::to_string(e), LogLevel::INFO);
    }

    void Scene::applyMovementEvent(const MovementEvent& ev)
    {
        if (ecs::has_component<ecs::Position>(reg, ev.getEntityId()))
        {
            auto& pos = ecs::get_component<ecs::Position>(reg, ev.getEntityId());
            pos.x += ev.getDeltaX();
            pos.y += ev.getDeltaY();
            log("[Scene] Moved entity " + std::to_string(ev.getEntityId()) +
                " by (" + std::to_string(ev.getDeltaX()) + "," +
                std::to_string(ev.getDeltaY()) + ")", LogLevel::INFO);
        }
    }

    std::unique_ptr<Scene> Scene::clone() const
    {
        auto newScene = std::make_unique<Scene>(logger, clock, sceneLogLevel);
        log("[Scene] Cloned scene", LogLevel::INFO);
        // TODO: deep copy ECS registry components
        return newScene;
    }

    void Scene::log(const std::string& msg, LogLevel lvl) const
    {
        if (logger && lvl >= sceneLogLevel)
        {
            logger->log(msg, lvl);
        }
    }
}
